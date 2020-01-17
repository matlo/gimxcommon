/*
 Copyright (c) 2016 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef ASYNC_H_

#define ASYNC_H_

#include <gimxpoll/include/gpoll.h>

typedef enum {
    E_ASYNC_DEVICE_TYPE_SERIAL,
    E_ASYNC_DEVICE_TYPE_HID,
} e_async_device_type;

typedef int (* ASYNC_READ_CALLBACK)(void * user, const void * buf, int status);
typedef int (* ASYNC_WRITE_CALLBACK)(void * user, int status);
typedef int (* ASYNC_CLOSE_CALLBACK)(void * user);
#ifndef WIN32
typedef GPOLL_REGISTER_FD ASYNC_REGISTER_SOURCE;
typedef GPOLL_REMOVE_FD ASYNC_REMOVE_SOURCE;
#else
typedef GPOLL_REGISTER_HANDLE ASYNC_REGISTER_SOURCE;
typedef GPOLL_REMOVE_HANDLE ASYNC_REMOVE_SOURCE;
#endif

typedef struct {
    ASYNC_READ_CALLBACK fp_read;       // called on data reception
    ASYNC_WRITE_CALLBACK fp_write;     // called on write completion
    ASYNC_CLOSE_CALLBACK fp_close;     // called on failure
    ASYNC_REGISTER_SOURCE fp_register; // to register device to event sources
    ASYNC_REMOVE_SOURCE fp_remove;     // to remove device from event sources
} ASYNC_CALLBACKS;

struct async_device;

struct async_device * async_open_path(const char * path, int print);
int async_close(struct async_device * device);
int async_read_timeout(struct async_device * device, void * buf, unsigned int count, unsigned int timeout);
int async_write_timeout(struct async_device * device, const void * buf, unsigned int count, unsigned int timeout);
int async_set_read_size(struct async_device * device, unsigned int size);
int async_register(struct async_device * device, void * user, const ASYNC_CALLBACKS * callbacks);
int async_write(struct async_device * device, const void * buf, unsigned int count);
int async_set_overlapped(struct async_device * device);

#ifdef WIN32
HANDLE * async_get_handle(struct async_device * device);
const char * async_get_path(struct async_device * device);
void async_set_device_type(struct async_device * device, e_async_device_type device_type);
int async_set_write_size(struct async_device * device, unsigned int size);
void async_set_private(struct async_device * device, void * priv);
void * async_get_private(struct async_device * device);
#else
int async_get_fd(struct async_device * device);
#endif

#endif /* ASYNC_H_ */
