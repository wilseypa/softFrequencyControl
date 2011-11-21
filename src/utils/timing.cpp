#include "utils/timing.h"

int diff_TIME( TIME &res, TIME &x, TIME &y ) {
//    if( x.FRAC < y.FRAC ) {
//        long int sec = ( y.FRAC - x.FRAC ) / FRAC_SEC_TIME + 1;
//        y.FRAC -= FRAC_SEC_TIME * sec;
//        y.tv_sec += sec;
//    }
//
//    if( x.FRAC - y.FRAC > FRAC_SEC_TIME ) {
//        long int sec = ( x.FRAC - y.FRAC ) / FRAC_SEC_TIME;
//        y.FRAC += FRAC_SEC_TIME * sec;
//        y.tv_sec -= sec;
//    }

    uint64_t x_frac = x.FRAC + ( FRAC_SEC_TIME * x.tv_sec );
    uint64_t y_frac = y.FRAC + ( FRAC_SEC_TIME * y.tv_sec );


//    res.tv_sec = x.tv_sec - y.tv_sec;
//    res.FRAC = x.FRAC - y.FRAC;
    x_frac -= y_frac;
    res.tv_sec = x_frac / FRAC_SEC_TIME;
    res.FRAC = x_frac % FRAC_SEC_TIME;

    return x.tv_sec < y.tv_sec;
}

int sum_TIME( TIME &res, TIME &x, TIME &y ) {
//    long int nsec = x.FRAC + y.FRAC;
//    long int sec = x.tv_sec + y.tv_sec;
//
//    if( nsec > FRAC_SEC_TIME ) {
//        sec += nsec / FRAC_SEC_TIME;
//        nsec = nsec % FRAC_SEC_TIME;
//    }
//
//    res.tv_sec = sec;
//    res.FRAC = nsec;

    uint64_t x_frac = x.FRAC + ( FRAC_SEC_TIME * x.tv_sec );
    uint64_t y_frac = y.FRAC + ( FRAC_SEC_TIME * y.tv_sec );

    x_frac += y_frac;

    res.tv_sec = x_frac / FRAC_SEC_TIME;
    res.FRAC = x_frac % FRAC_SEC_TIME;

    return 0;
}

//int avg_TIME( TIME &avg, TIME &tot, int samples ) {
//    avg.tv_sec = tot.tv_sec / samples;
//    avg.FRAC = (tot.FRAC + (tot.tv_sec % samples) * FRAC_SEC_TIME) / samples;
//    return 0;
//}

int avg_TIME( TIME &avg, TIME &tot, int samples ) {
    uint64_t tmp = tot.FRAC + ( FRAC_SEC_TIME * tot.tv_sec );
    tmp /= samples;

    avg.tv_sec = tmp / FRAC_SEC_TIME;
    avg.FRAC = tmp % FRAC_SEC_TIME;
}

int PrintTime( TIME &t ) {
    AdjustTime( t );
    return printf( TIME_PRINT, t.tv_sec, t.FRAC );
}

void AdjustTime( TIME &t ) {
    uint64_t x_frac = t.FRAC + ( FRAC_SEC_TIME * t.tv_sec );

    t.tv_sec = x_frac / FRAC_SEC_TIME;
    t.FRAC = x_frac % FRAC_SEC_TIME;
}

timespec convertTimeToTimespec( TIME &t ) {
    timespec _t;
    _t.tv_sec = t.tv_sec;
    #if NANO_TIME
        _t.tv_nsec = t.tv_nsec;
    #else
        _t.tv_nsec = t.tv_usec * 1000;
    #endif
    return _t;

}
