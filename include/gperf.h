/*
 Copyright (c) 2019 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef GPERF_H_
#define GPERF_H_

#include <gimxtime/include/gtime.h>
#include <gimxlog/include/glog.h>

#define GPERF_XSTR(s) GPERF_STR(s)
#define GPERF_STR(s) #s

#define GPERF_INST(NAME) \
    static gtime gperf_##NAME##_sum = 0; \
    static unsigned long long gperf_##NAME##_count = 0; \
    static gtime gperf_##NAME##_start = 0; \
    static gtime gperf_##NAME##_end = 0; \
    static gtime gperf_##NAME##_worst = 0; \
    static gtime gperf_##NAME##_diff = 0;

#define GPERF_START(NAME) \
    do { \
        gperf_##NAME##_start = gtime_gettime(); \
    } while (0)

#define GPERF_END(NAME) \
    do { \
        if (gperf_##NAME##_start != 0) { \
           gperf_##NAME##_end = gtime_gettime(); \
           gperf_##NAME##_diff = gperf_##NAME##_end - gperf_##NAME##_start; \
           if (gperf_##NAME##_diff > gperf_##NAME##_worst) { \
               gperf_##NAME##_worst = gperf_##NAME##_diff; \
           } \
           gperf_##NAME##_sum += gperf_##NAME##_diff; \
           gperf_##NAME##_count += 1; \
        } \
    } while (0)

#define GPERF_TICK(NAME,TIME) \
    do { \
        gperf_##NAME##_end = TIME; \
        if (gperf_##NAME##_start != 0) { \
           gperf_##NAME##_diff = gperf_##NAME##_end - gperf_##NAME##_start; \
           if (gperf_##NAME##_diff > gperf_##NAME##_worst) { \
               gperf_##NAME##_worst = gperf_##NAME##_diff; \
           } \
           gperf_##NAME##_sum += gperf_##NAME##_diff; \
           gperf_##NAME##_count += 1; \
        } \
        gperf_##NAME##_start = gperf_##NAME##_end; \
    } while (0)

#define GPERF_LOG(NAME) \
    do { \
        if (GLOG_LEVEL(GLOG_NAME,DEBUG) && gperf_##NAME##_count) { \
            printf(GPERF_XSTR(NAME)": count = %I64u, average = %I64u, worst = %I64u\n", gperf_##NAME##_count, gperf_##NAME##_sum / gperf_##NAME##_count, gperf_##NAME##_worst); \
        } \
    } while (0)

#endif /* GPERF_H_ */
