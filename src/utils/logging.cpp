#include "utils/logging.h"

#include <cstdio>

void printFrequencyTable( map<int, vector<TIME> > &speed_v_lapses, int samplings ) {
    map<int, vector<TIME> >::iterator speed_it;
    vector<TIME>::iterator tick_it;

    // print table header
//    printf("#FREQUENCY\tTime Stamp 1\tTime Stamp 2\tDifference\tRunning Total\n");
    printf ("#Frequency\tBegin (%s)\tEnd (%s)\n", TIME_ABRV, TIME_ABRV);
    TIME total, avg, diff, tick1, tick2;

    for( speed_it = speed_v_lapses.begin(); speed_it != speed_v_lapses.end(); speed_it++ ) {
        printf( "%d", speed_it->first);
//        total.tv_sec = 0;
//        total.FRAC = 0;

        for( tick_it = speed_it->second.begin(); tick_it != speed_it->second.end(); tick_it++ ) {
            tick1 = *tick_it;
            tick_it++;
            tick2 = *tick_it;
//            diff_TIME( diff, tick2, tick1 );
//            sum_TIME( total, total, diff );
            //printf( "\t%d.%09d\t%d.%09d\t%d.%09d\t%d.%09d\n", tick1.tv_sec, tick1.FRAC, tick2.tv_sec, tick2.FRAC, diff.tv_sec, diff.FRAC, total.tv_sec, total.FRAC );
            printf( "\t" );
//            printf( TIME_PRINT, tick1.tv_sec, tick1.FRAC );
            PrintTime(tick1);
            printf( "\t" );
//            printf( TIME_PRINT, tick2.tv_sec, tick2.FRAC );
            PrintTime(tick2);
//            printf( "\t" );
//            printf( TIME_PRINT, diff.tv_sec, diff.FRAC );
//            PrintTime(diff);
//            printf( "\t" );
//            printf( TIME_PRINT, total.tv_sec, total.FRAC );
//            PrintTime(total);
            printf( "\n" );
        }

//        avg_TIME( avg, total, samplings );
//        printf( "Average\t" );
//        printf( TIME_PRINT, avg.tv_sec, avg.FRAC );
//        PrintTime(avg);
//        printf( "\n" );
    }
}

void printFrequencyTable2( map<int, vector<TIME> > &speed_v_lapses, map<int, vector<uint64_t> > &loop_counts, int samplings ) {
    map<int, vector<TIME> >::iterator speed_it;
    map<int, vector<uint64_t> >::iterator lcnt_it;
    vector<TIME>::iterator tick_it;
    vector<uint64_t>::iterator loop_it;

    // print table header
    printf("#FREQUENCY\tBegin (%s)\tEnd (%s)\tNodes Visited\n", TIME_ABRV, TIME_ABRV);
    TIME total, avg, diff, tick1, tick2;

    for( speed_it = speed_v_lapses.begin(); speed_it != speed_v_lapses.end(); speed_it++ ) {
        printf( "%d", speed_it->first);
//        total.tv_sec = 0;
//        total.FRAC = 0;

        lcnt_it = loop_counts.find(speed_it->first);

        for( tick_it = speed_it->second.begin(), loop_it = lcnt_it->second.begin(); tick_it != speed_it->second.end(); tick_it++, loop_it++ ) {
            tick1 = *tick_it;
            tick_it++;
            tick2 = *tick_it;
//            diff_TIME( diff, tick2, tick1 );
//            sum_TIME( total, total, diff );
            //printf( "\t%d.%09d\t%d.%09d\t%d.%09d\t%d.%09d\n", tick1.tv_sec, tick1.FRAC, tick2.tv_sec, tick2.FRAC, diff.tv_sec, diff.FRAC, total.tv_sec, total.FRAC );
            printf( "\t" );
//            printf( TIME_PRINT, tick1.tv_sec, tick1.FRAC );
            PrintTime(tick1);
            printf( "\t" );
//            printf( TIME_PRINT, tick2.tv_sec, tick2.FRAC );
            PrintTime(tick2);
//            printf( "\t" );
//            printf( TIME_PRINT, diff.tv_sec, diff.FRAC );
//            PrintTime(diff);
//            printf( "\t" );
//            printf( TIME_PRINT, total.tv_sec, total.FRAC );
//            PrintTime(total);
            printf( "\t%lu", *loop_it);
            printf( "\n" );
        }

//        avg_TIME( avg, total, samplings );
//        printf( "Average\t" );
//        printf( TIME_PRINT, avg.tv_sec, avg.FRAC );
//        PrintTime(avg);
//        printf( "\n" );
    }
}
