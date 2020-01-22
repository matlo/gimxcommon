/*
 Copyright (c) 2016 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#include "../../include/async.h"
#include "../../include/gerror.h"
#include "../../include/glist.h"
#include <gimxlog/include/glog.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define ASYNC_MAX_WRITE_QUEUE_SIZE 2

GLOG_GET(GLOG_NAME)

typedef struct
{
  struct
  {
    char * buf;
    unsigned int count;
  } data[ASYNC_MAX_WRITE_QUEUE_SIZE];
  unsigned int nb;
} s_queue;

struct async_device {
    HANDLE handle;
    char * path;
    struct
    {
      OVERLAPPED overlapped;
      char * buf;
      unsigned int count;
      unsigned int bread;
      unsigned int size;
    } read;
    struct
    {
      OVERLAPPED overlapped;
      s_queue queue;
      unsigned int size;
    } write;
    struct {
        void * user;
        ASYNC_READ_CALLBACK fp_read;
        ASYNC_WRITE_CALLBACK fp_write;
        ASYNC_CLOSE_CALLBACK fp_close;
        ASYNC_REMOVE_SOURCE fp_remove;
    } callback;
    void * priv;
    e_async_device_type device_type;
    GLIST_LINK(struct async_device);
};

static GLIST_INST(struct async_device, async_devices);

static struct async_device * add_device(const char * path, HANDLE handle, int print) {

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
    device->handle = handle;
    device->read.overlapped.hEvent = INVALID_HANDLE_VALUE;
    device->write.overlapped.hEvent = INVALID_HANDLE_VALUE;
    GLIST_ADD(async_devices, device);
    return device;
}

static int queue_write(struct async_device * device, const char * buf, unsigned int count) {
  if(device->write.queue.nb == ASYNC_MAX_WRITE_QUEUE_SIZE) {
      if (GLOG_LEVEL(GLOG_NAME,ERROR)) {
          fprintf(stderr, "%s:%d %s: no space left in write queue for device (%s)\n", __FILE__, __LINE__, __func__, device->path);
      }
      return -1;
  }
  if(count < device->write.size) {
      count = device->write.size;
  }
  void * dup = calloc(count, sizeof(char));
  if(!dup) {
      PRINT_ERROR_ALLOC_FAILED("malloc");
      return -1;
  }
  memcpy(dup, buf, count);
  device->write.queue.data[device->write.queue.nb].buf = dup;
  device->write.queue.data[device->write.queue.nb].count = count;
  ++device->write.queue.nb;
  return device->write.queue.nb - 1;
}

static int dequeue_write(struct async_device * device) {
  if(device->write.queue.nb > 0) {
      --device->write.queue.nb;
      free(device->write.queue.data[0].buf);
      memmove(device->write.queue.data, device->write.queue.data + 1, device->write.queue.nb * sizeof(*device->write.queue.data));
  }
  return device->write.queue.nb - 1;
}

static int set_overlapped(struct async_device * device) {
    /*
     * create event objects for overlapped I/O
     */
    device->read.overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(device->read.overlapped.hEvent == INVALID_HANDLE_VALUE) {
        PRINT_ERROR_GETLASTERROR("CreateEvent");
        return -1;
    }
    device->write.overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(device->write.overlapped.hEvent == INVALID_HANDLE_VALUE) {
        PRINT_ERROR_GETLASTERROR("CreateEvent");
        return -1;
    }
    return 0;
}

struct async_device * async_open_path(const char * path, int print) {

    DWORD accessdirection = GENERIC_READ | GENERIC_WRITE;
    DWORD sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    struct async_device * device = NULL;
    if(path != NULL) {
        HANDLE handle = CreateFile(path, accessdirection, sharemode, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
        if(handle != INVALID_HANDLE_VALUE) {
            device = add_device(path, handle, print);
            if(device == NULL) {
                CloseHandle(handle);
            }
            else if(set_overlapped(device) == -1) {
                async_close(device);
                device = NULL;
            }
        }
        else {
            if(print) {
                PRINT_ERROR_GETLASTERROR("CreateFile");
            }
        }
    }
    return device;
}

int async_close(struct async_device * device) {

    DWORD dwBytesTransfered;

    if(device->read.overlapped.hEvent != INVALID_HANDLE_VALUE) {
      if(CancelIoEx(device->handle, &device->read.overlapped)) {
        if (!GetOverlappedResult(device->handle, &device->read.overlapped, &dwBytesTransfered, TRUE)) { //block until completion
          if(GetLastError() != ERROR_OPERATION_ABORTED) {
            PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
          }
        }
      }
      else if(GetLastError() != ERROR_NOT_FOUND)
      {
        PRINT_ERROR_GETLASTERROR("CancelIoEx");
      }
    }
    if(device->write.overlapped.hEvent != INVALID_HANDLE_VALUE) {
      if(CancelIoEx(device->handle, &device->write.overlapped)) {
        if (!GetOverlappedResult(device->handle, &device->write.overlapped, &dwBytesTransfered, TRUE)) { //block until completion
          if(GetLastError() != ERROR_OPERATION_ABORTED) {
            PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
          }
        }
      }
      else if(GetLastError() != ERROR_NOT_FOUND)
      {
        PRINT_ERROR_GETLASTERROR("CancelIoEx");
      }
    }

    while(dequeue_write(device) != -1) ;

    free(device->read.buf);
    free(device->path);

    if (device->callback.fp_remove != NULL) {
        device->callback.fp_remove(device->read.overlapped.hEvent);
        device->callback.fp_remove(device->write.overlapped.hEvent);
    }

    CloseHandle(device->read.overlapped.hEvent);
    CloseHandle(device->write.overlapped.hEvent);
    CloseHandle(device->handle);

    GLIST_REMOVE(async_devices, device);

    free(device);

    return 0;
}

int async_read_timeout(struct async_device * device, void * buf, unsigned int count, unsigned int timeout) {

  if(device->device_type == E_ASYNC_DEVICE_TYPE_HID && device->read.size == 0) {
    PRINT_ERROR_OTHER("the HID device has no IN endpoint");
    return -1;
  }

  DWORD dwBytesRead = 0;

  memset(buf, 0x00, count);

  char * dest = NULL;
  unsigned int destLength = 0;

  switch (device->device_type) {
  case E_ASYNC_DEVICE_TYPE_HID:
      dest = device->read.buf;
      destLength = device->read.count;
      break;
  case E_ASYNC_DEVICE_TYPE_SERIAL:
      dest = buf;
      destLength = count;
      break;
  }

  if(!ReadFile(device->handle, dest, destLength, NULL, &device->read.overlapped)) {
    if(GetLastError() != ERROR_IO_PENDING) {
      PRINT_ERROR_GETLASTERROR("ReadFile");
      return -1;
    }
    int ret = WaitForSingleObject(device->read.overlapped.hEvent, timeout);
    switch (ret)
    {
      case WAIT_OBJECT_0:
        if (!GetOverlappedResult(device->handle, &device->read.overlapped, &dwBytesRead, FALSE)) {
          PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
          return -1;
        }
        break;
      case WAIT_TIMEOUT:
        if(!CancelIoEx(device->handle, &device->read.overlapped)) {
          PRINT_ERROR_GETLASTERROR("CancelIoEx");
          return -1;
        }
        if (!GetOverlappedResult(device->handle, &device->read.overlapped, &dwBytesRead, TRUE)) { //block until completion
          if (GetLastError() != ERROR_OPERATION_ABORTED) {
            PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
            return -1;
          }
        }
        if(dwBytesRead != destLength) { // the read operation may still have succeed
          if (GLOG_LEVEL(GLOG_NAME,DEBUG)) {
            fprintf(stderr, "%s:%d %s: ReadFile failed: timeout expired.\n", __FILE__, __LINE__, __func__);
          }
        }
        break;
      default:
        PRINT_ERROR_GETLASTERROR("WaitForSingleObject");
        return -1;
    }
  }
  else
  {
    dwBytesRead = destLength;
  }
  
  int length = dwBytesRead;

  // skip the eventual leading null byte for hid devices
  if (device->device_type == E_ASYNC_DEVICE_TYPE_HID && dwBytesRead > 0) {
    if (dest[0] == 0x00) {
      --dwBytesRead;
      length = dwBytesRead > count ? count : dwBytesRead;
      memcpy(buf, dest + 1, length);
    } else {
      length = dwBytesRead > count ? count : dwBytesRead;
      memcpy(buf, dest, dwBytesRead);
    }
  }

  return length;
}

int async_write_timeout(struct async_device * device, const void * buf, unsigned int count, unsigned int timeout) {
    
  if(device->device_type == E_ASYNC_DEVICE_TYPE_HID && device->write.size == 0) {
    PRINT_ERROR_OTHER("the HID device has no OUT endpoint");
    return -1;
  }

  DWORD dwBytesWritten = 0;

  if(count < device->write.size) {
      count = device->write.size;
  }

  if(!WriteFile(device->handle, buf, count, NULL, &device->write.overlapped)) {
    if(GetLastError() != ERROR_IO_PENDING) {
      PRINT_ERROR_GETLASTERROR("WriteFile");
      return -1;
    }
    int ret = WaitForSingleObject(device->write.overlapped.hEvent, timeout);
    switch (ret) {
      case WAIT_OBJECT_0:
        if (!GetOverlappedResult(device->handle, &device->write.overlapped, &dwBytesWritten, FALSE)) {
          PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
          return -1;
        }
        break;
      case WAIT_TIMEOUT:
        if(!CancelIoEx(device->handle, &device->write.overlapped)) {
          PRINT_ERROR_GETLASTERROR("CancelIoEx");
          return -1;
        }
        if (!GetOverlappedResult(device->handle, &device->write.overlapped, &dwBytesWritten, TRUE)) { //block until completion
          if (GetLastError() != ERROR_OPERATION_ABORTED) {
            PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
            return -1;
          }
        }
        if(dwBytesWritten != count) { // the write operation may still have succeed
          PRINT_ERROR_OTHER("WriteFile failed: timeout expired.");
        }
        break;
      default:
        PRINT_ERROR_GETLASTERROR("WaitForSingleObject");
        return -1;
    }
  }
  else {
    dwBytesWritten = count;
  }

  return dwBytesWritten;
}

/*
 * This function starts an overlapped read.
 * If the read completes immediately, it returns the number of transferred bytes, which is the number of requested bytes.
 * If the read is pending, it returns -1.
 */
static int start_overlapped_read(struct async_device * device) {

  if(device->read.buf == NULL) {

    PRINT_ERROR_OTHER("the read buffer is NULL");
    return -1;
  }

  int ret = -1;

  memset(device->read.buf, 0x00, device->read.count);
  
  if(!ReadFile(device->handle, device->read.buf, device->read.count, NULL, &device->read.overlapped))
  {
    if(GetLastError() != ERROR_IO_PENDING)
    {
      PRINT_ERROR_GETLASTERROR("ReadFile");
      ret = -1;
    }
  }
  else
  {
    DWORD dwBytesRead;
    if (!GetOverlappedResult(device->handle, &device->read.overlapped, &dwBytesRead, FALSE)) {
      PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
      return -1;
    }
    // the read immediately completed
    ret = device->read.bread = dwBytesRead;
  }
  
  return ret;
}

static int read_packet(struct async_device * device) {
  
  if(device->read.bread) {
    if(device->callback.fp_read != NULL) {
      // skip the eventual leading null byte for hid devices
      if (device->device_type == E_ASYNC_DEVICE_TYPE_HID && ((unsigned char*)device->read.buf)[0] == 0x00) {
        --device->read.bread;
        memmove(device->read.buf, device->read.buf + 1, device->read.bread);
      }
      device->callback.fp_read(device->callback.user, (const char *)device->read.buf, device->read.bread);
    }
    device->read.bread = 0;
  }

  return start_overlapped_read(device);
}

static int read_callback(void * user) {
    
    struct async_device * device = (struct async_device *) user;
    
    DWORD dwBytesRead = 0;

    if (!GetOverlappedResult(device->handle, &device->read.overlapped, &dwBytesRead, FALSE))
    {
      PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
      return -1;
    }

    device->read.bread = dwBytesRead;
    
    while(read_packet(device) >= 0) ;

    return 0;
}

static int write_internal(struct async_device * device) {

    DWORD dwBytesWritten = 0;

    if(!WriteFile(device->handle, device->write.queue.data[0].buf, device->write.queue.data[0].count, NULL, &device->write.overlapped)) {
      if(GetLastError() != ERROR_IO_PENDING) {
        PRINT_ERROR_GETLASTERROR("WriteFile");
        return -1;
      }
    }
    else {
      if (!GetOverlappedResult(device->handle, &device->write.overlapped, &dwBytesWritten, FALSE)) {
        PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
        return -1;
      }
    }

    return dwBytesWritten;
}

static int write_callback(void * user) {

    struct async_device * device = (struct async_device *) user;

    int ret = 0;

    DWORD dwBytesWritten = 0;

    if (!GetOverlappedResult(device->handle, &device->write.overlapped, &dwBytesWritten, FALSE))
    {
      PRINT_ERROR_GETLASTERROR("GetOverlappedResult");
      ret = -1;
    }

    while(dequeue_write(device) != -1) {
        int result = write_internal(device);
        if(result == 0) {
            break; // IO is pending, completion will execute write_callback
        }
        else if(result < 0) {
            ret = -1; // IO failed, report this error
        }
    }

    if(device->callback.fp_write) {
        device->callback.fp_write(device->callback.user, ret == 0 ? (int)dwBytesWritten : -1);
    }

    return ret;
}

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
    
    while(read_packet(device) >= 0) ;

    int ret = 0;

    if (callbacks->fp_read) {
        GPOLL_CALLBACKS gpoll_callbacks = {
          .fp_read = read_callback,
          .fp_write = NULL,
          .fp_close = close_callback,
        };
        ret = callbacks->fp_register(device->read.overlapped.hEvent, device, &gpoll_callbacks);
    }
    if (ret != -1) {
        GPOLL_CALLBACKS gpoll_callbacks = {
          .fp_read = NULL,
          .fp_write = write_callback,
          .fp_close = close_callback,
        };
        ret = callbacks->fp_register(device->write.overlapped.hEvent, device, &gpoll_callbacks);
    }

    if (ret != -1) {
      device->callback.user = user;
      device->callback.fp_read = callbacks->fp_read;
      device->callback.fp_write = callbacks->fp_write;
      device->callback.fp_close = callbacks->fp_close;
      device->callback.fp_remove = callbacks->fp_remove;
    }

    return ret;
}

int async_write(struct async_device * device, const void * buf, unsigned int count) {
    
    if(device->device_type == E_ASYNC_DEVICE_TYPE_HID && device->write.size == 0) {
        PRINT_ERROR_OTHER("the HID device has no OUT endpoint");
        return -1;
    }

    int res = queue_write(device, buf, count);
    if(res < 0) {
        return -1; // cannot be queued
    }
    else if(res > 0) {
        return 0; // another IO is pending
    }

    int ret = write_internal(device);
    if(ret != 0) {
        dequeue_write(device); // IO failed or completed
        //TODO MLA: manage disconnection
    }

    return ret;
}

void async_set_private(struct async_device * device, void * priv) {

    device->priv = priv;
}

void * async_get_private(struct async_device * device) {

    return device->priv;
}

HANDLE * async_get_handle(struct async_device * device) {

    return device->handle;
}

int async_set_write_size(struct async_device * device, unsigned int size) {

    device->write.size = size;

    return 0;
}

void async_set_device_type(struct async_device * device, e_async_device_type device_type) {

    device->device_type = device_type;
}

const char * async_get_path(struct async_device * device) {

    return device->path;
}
