/*
 Copyright (c) 2016 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#include "../../include/async.h"
#include "../../include/gerror.h"
#include "../../include/glist.h"
#include "gimxlog/include/glog.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

GLOG_GET(GLOG_NAME)

struct async_device {
    int fd;
    char * path;
    struct
    {
      char * buf;
      unsigned int count;
      unsigned int bread;
      unsigned int size;
    } read;
    struct {
        void * user;
        ASYNC_READ_CALLBACK fp_read;
        ASYNC_WRITE_CALLBACK fp_write;
        ASYNC_CLOSE_CALLBACK fp_close;
        ASYNC_REMOVE_SOURCE fp_remove;
    } callback;
    void * priv;
    GLIST_LINK(struct async_device);
};

static GLIST_INST(struct async_device, async_devices);

static struct async_device * add_device(const char * path, int fd, int print) {

    struct async_device * current = GLIST_BEGIN(async_devices);
    while (current != GLIST_END(async_devices)) {
        if(current->path && !strcmp(current->path, path)) {
            if(print) {
                if (GLOG_LEVEL(GLOG_NAME,ERROR)) {
                    fprintf(stderr, "%s:%d add_device %s: device already opened\n", __FILE__, __LINE__, path);
                }
            }
            return NULL;
        }
        current = current->next;
    }

    struct async_device * device = calloc(1, sizeof(*device));
    if (device == NULL) {
        PRINT_ERROR_ALLOC_FAILED("calloc");
        return NULL;
    }
    device->path = strdup(path);
    if (device->path == NULL) {
        PRINT_ERROR_OTHER("failed to duplicate path");
        free(device);
        return NULL;
    }
    device->fd = fd;
    GLIST_ADD(async_devices, device);
    return device;
}

struct async_device * async_open_path(const char * path, int print) {

    struct async_device * device = NULL;
    if(path != NULL) {
        int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if(fd != -1) {
            device = add_device(path, fd, print);
            if(device == NULL) {
                close(fd);
            }
        }
        else {
            if(print) {
                PRINT_ERROR_ERRNO("open");
            }
        }
    }
    return device;
}

int async_close(struct async_device * device) {

    if (device->callback.fp_remove != NULL) {
        device->callback.fp_remove(device->fd);
    }

    close(device->fd);

    free(device->path);
    free(device->read.buf);

    GLIST_REMOVE(async_devices, device);

    free(device);

    return 0;
}

int async_read_timeout(struct async_device * device, void * buf, unsigned int count, unsigned int timeout) {

  unsigned int bread = 0;
  int res;

  fd_set readfds;

  time_t sec = timeout / 1000;
  __suseconds_t usec = (timeout - sec * 1000) * 1000;
  struct timeval tv = {.tv_sec = sec, .tv_usec = usec};

  while(bread != count)
  {
    FD_ZERO(&readfds);
    FD_SET(device->fd, &readfds);
    int status = select(device->fd+1, &readfds, NULL, NULL, &tv);
    if(status > 0)
    {
      if(FD_ISSET(device->fd, &readfds))
      {
        res = read(device->fd, buf+bread, count-bread);
        if(res > 0)
        {
          bread += res;
        }
      }
    }
    else if (status < 0)
    {
      if(errno == EINTR)
      {
        continue;
      }
      PRINT_ERROR_ERRNO("select");
    }
    else
    {
      break; // timeout
    }
  }

  return bread;
}

int async_write_timeout(struct async_device * device, const void * buf, unsigned int count, unsigned int timeout) {

  unsigned int bwritten = 0;
  int res;

  fd_set writefds;

  time_t sec = timeout / 1000;
  __suseconds_t usec = (timeout - sec * 1000) * 1000;
  struct timeval tv = {.tv_sec = sec, .tv_usec = usec};

  while(bwritten != count)
  {
    FD_ZERO(&writefds);
    FD_SET(device->fd, &writefds);
    int status = select(device->fd+1, NULL, &writefds, NULL, &tv);
    if(status > 0)
    {
      if(FD_ISSET(device->fd, &writefds))
      {
        res = write(device->fd, buf+bwritten, count-bwritten);
        if(res > 0)
        {
          bwritten += res;
        }
      }
    }
    else if(errno == EINTR)
    {
      continue;
    }
    else
    {
      PRINT_ERROR_ERRNO("select");
      break;
    }
  }

  return bwritten;
}

/*
 * This function is called on data reception.
 */
static int read_callback(void * user) {

    struct async_device * device = (struct async_device *) user;

    int ret = read(device->fd, device->read.buf, device->read.count);

    if(ret < 0) {
        PRINT_ERROR_ERRNO("read");
    }

    return device->callback.fp_read(device->callback.user, (const char *)device->read.buf, ret);
}

/*
 * This function is called on failure.
 */
static int close_callback(void * user) {

    struct async_device * device = (struct async_device *) user;

    return device->callback.fp_close(device->callback.user);
}

int async_set_read_size(struct async_device * device, unsigned int size) {

    if(size > device->read.size) {
        void * ptr = realloc(device->read.buf, size);
        if(ptr == NULL) {
            PRINT_ERROR_ALLOC_FAILED("realloc");
            return -1;
        }
        device->read.buf = ptr;
        device->read.size = size;
    }

    device->read.count = size;

    return 0;
}

int async_register(struct async_device * device, void * user, const ASYNC_CALLBACKS * callbacks) {

    if (callbacks->fp_remove == NULL) {
        PRINT_ERROR_OTHER("fp_remove is NULL");
    }

    if (callbacks->fp_register == NULL) {
        PRINT_ERROR_OTHER("fp_register is NULL");
    }

    device->callback.user = user;
    device->callback.fp_read = callbacks->fp_read;
    //fp_write is ignored
    device->callback.fp_close = callbacks->fp_close;
    device->callback.fp_remove = callbacks->fp_remove;

    GPOLL_CALLBACKS gpoll_callbacks = {
            .fp_read = read_callback,
            .fp_write = NULL,
            .fp_close = close_callback,
    };
    return callbacks->fp_register(device->fd, device, &gpoll_callbacks);
}

int async_write(struct async_device * device, const void * buf, unsigned int count) {

    int ret = write(device->fd, buf, count);
    if (ret == -1) {
        PRINT_ERROR_ERRNO("write");
    }
    else if((unsigned int) ret != count) {
        if (GLOG_LEVEL(GLOG_NAME,ERROR)) {
            fprintf(stderr, "%s:%d write: only %u written (requested %u)\n", __FILE__, __LINE__, ret, count);
        }
    }

    return ret;
}

int async_get_fd(struct async_device * device) {

    return device->fd;
}
