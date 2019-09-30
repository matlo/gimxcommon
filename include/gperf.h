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

#define GPERF_INST(NAME, SAMPLETYPE, MAXSAMPLES) \
   struct { \
      gtime sum; \
      unsigned long long count; \
      gtime start; \
      gtime end; \
      gtime worst; \
      gtime diff; \
      SAMPLETYPE samples[MAXSAMPLES]; \
      unsigned int maxsamples; \
      unsigned int last; \
      int wrapped; \
   } gperf_##NAME = { .maxsamples = MAXSAMPLES }

#define GPERF_SAMPLE(NAME) \
   gperf_##NAME.samples[gperf_##NAME.last]

#define GPERF_SAMPLE_INC(NAME) \
   do { \
      gperf_##NAME.last++; \
      if (gperf_##NAME.last == gperf_##NAME.maxsamples) { \
         gperf_##NAME.last = 0; \
         gperf_##NAME.wrapped = 1; \
      } \
   } while (0)

#define GPERF_SAMPLE_PRINT(NAME, SAMPLEPRINT) \
   do { \
      unsigned int gperf##NAME_first = gperf_##NAME.wrapped ? gperf_##NAME.last : 0; \
      unsigned int gperf##NAME_nb = gperf_##NAME.wrapped ? gperf_##NAME.maxsamples : gperf_##NAME.last; \
      unsigned int gperf##NAME_index; \
      for (gperf##NAME_index = gperf##NAME_first; gperf##NAME_index < gperf##NAME_first + gperf##NAME_nb; ++gperf##NAME_index) { \
          unsigned int gperf##NAME_index_r = gperf##NAME_index % gperf_##NAME.maxsamples; \
          SAMPLEPRINT(gperf_##NAME.samples[gperf##NAME_index_r]); \
      } \
   } while (0)

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
            printf(GPERF_XSTR(NAME)": count = "GTIME_FS", average = "GTIME_FS", worst = "GTIME_FS"\n", gperf_##NAME.count, gperf_##NAME.sum / gperf_##NAME.count, gperf_##NAME.worst); \
        } \
    } while (0)

#endif /* GPERF_H_ */
