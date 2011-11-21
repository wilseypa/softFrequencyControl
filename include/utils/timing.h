#ifndef TIMING_H_INCLUDED
#define TIMING_H_INCLUDED

#include <sys/time.h>
#include <stdint.h>
#include <cstdio>

#ifndef NANO_TIME
#define NANO_TIME 0
#endif

#if NANO_TIME

#define FRAC_SEC_TIME 1000000000
#define FRAC tv_nsec
#define TIME_PRINT "%lu.%09lu"
#define TIME_ABRV "ns"

#define GetTime(x) clock_gettime(CLOCK_MONOTONIC, &x)
typedef timespec TIME;

#else

#define FRAC_SEC_TIME 1000000
#define FRAC tv_usec
#define TIME_PRINT "%lu.%06lu"
#define TIME_ABRV "us"

#define GetTime(x) gettimeofday(&x, NULL)
typedef timeval TIME;

#endif

//#define PrintTime(x) printf(TIME_PRINT, x.tv_sec, x.FRAC)

int diff_TIME( TIME &res, TIME &x, TIME &y );
int sum_TIME( TIME &res, TIME &x, TIME &y );
int avg_TIME( TIME &avg, TIME &tot, int samples );

void AdjustTime(TIME &t);
int PrintTime(TIME &t);
timespec convertTimeToTimespec( TIME &t);

#endif // TIMING_H_INCLUDED
