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
  if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
    fprintf(stderr, "%s:%d %s: %s failed with error", __FILE__, __LINE__, __func__, msg); \
    gerror_print_last(""); \
  }
#endif

#define PRINT_ERROR_ERRNO(msg) \
  if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
    fprintf(stderr, "%s:%d %s: %s failed with error: %m\n", __FILE__, __LINE__, __func__, msg); \
  }

#define PRINT_ERROR_ALLOC_FAILED(func) \
  if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
    fprintf(stderr, "%s:%d %s: %s failed\n", __FILE__, __LINE__, __func__, func); \
  }

#define PRINT_ERROR_OTHER(msg) \
  if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
    fprintf(stderr, "%s:%d %s: %s\n", __FILE__, __LINE__, __func__, msg); \
  }

#endif /* GERROR_H_ */
