/*
 Copyright (c) 2019 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef GPERF_H_
#define GPERF_H_

#include <gimxtime/include/gtime.h>
#include <gimxlog/include/glog.h>

#include <inttypes.h>

#define GPERF_XSTR(s) GPERF_STR(s)
#define GPERF_STR(s) #s

#define GPERF_INST(NAME) \
    struct { \
        gtime sum; \
        unsigned long long count; \
        gtime start; \
        gtime end; \
        gtime worst; \
        gtime diff; \
    } gperf_##NAME = {}

#define GPERF_START(NAME) \
    do { \
        gperf_##NAME.start = gtime_gettime(); \
    } while (0)

#define GPERF_END(NAME) \
    do { \
        if (gperf_##NAME.start != 0) { \
           gperf_##NAME.end = gtime_gettime(); \
           gperf_##NAME.diff = gperf_##NAME.end - gperf_##NAME.start; \
           if (gperf_##NAME.diff > gperf_##NAME.worst) { \
               gperf_##NAME.worst = gperf_##NAME.diff; \
           } \
           gperf_##NAME.sum += gperf_##NAME.diff; \
           gperf_##NAME.count += 1; \
        } \
    } while (0)

#define GPERF_TICK(NAME, TIME) \
    do { \
        gperf_##NAME.end = TIME; \
        if (gperf_##NAME.start != 0) { \
           gperf_##NAME.diff = gperf_##NAME.end - gperf_##NAME.start; \
           if (gperf_##NAME.diff > gperf_##NAME.worst) { \
               gperf_##NAME.worst = gperf_##NAME.diff; \
           } \
           gperf_##NAME.sum += gperf_##NAME.diff; \
           gperf_##NAME.count += 1; \
        } \
        gperf_##NAME.start = gperf_##NAME.end; \
    } while (0)

#define GPERF_LOG(NAME) \
    do { \
        if (gperf_##NAME.count) { \
            printf(GPERF_XSTR(NAME)": count = %"PRIu64", average = %"PRIu64", worst = %"PRIu64"\n", gperf_##NAME.count, gperf_##NAME.sum / gperf_##NAME.count, gperf_##NAME.worst); \
        } \
    } while (0)

#endif /* GPERF_H_ */
