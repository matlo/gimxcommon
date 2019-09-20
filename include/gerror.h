/*
 Copyright (c) 2019 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef GERROR_H_
#define GERROR_H_

#include <stdio.h>

#ifdef WIN32
void gerror_print_last(const char * msg);

#define PRINT_ERROR_GETLASTERROR(msg) \
    do { \
        if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
            fprintf(stderr, "%s:%d %s: %s failed with error", __FILE__, __LINE__, __func__, msg); \
            gerror_print_last(""); \
        } \
    } while (0)
#endif

#define PRINT_ERROR_ERRNO(msg) \
    do { \
        if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
            fprintf(stderr, "%s:%d %s: %s failed with error: %m\n", __FILE__, __LINE__, __func__, msg); \
        } \
    } while (0)

#define PRINT_ERROR_ALLOC_FAILED(func) \
    do { \
        if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
            fprintf(stderr, "%s:%d %s: %s failed\n", __FILE__, __LINE__, __func__, func); \
        } \
    } while (0)

#define PRINT_ERROR_OTHER(msg) \
    do { \
        if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
            fprintf(stderr, "%s:%d %s: %s\n", __FILE__, __LINE__, __func__, msg); \
        } \
    } while (0)

#endif /* GERROR_H_ */
