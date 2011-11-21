#include <iostream>
#include <cstdio>
#include <map>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unistd.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>



#include <pthread.h>
#include <sched.h>

#include "utils/timing.h"
#include "utils/cpufunc.h"
#include "utils/logging.h"

using namespace std;

//vector<TIME> simple_times;
//vector<TIME> medium_times;
//vector<TIME> slow_times;

void ThreadTimingTest ( map<int, string > &start_v_end, int samplings, int num_threads );
void SpeedTest ( map<int, vector<TIME> > &speed_v_lapse,  void * ( *algo ) ( void * ),
                 pthread_attr_t &thread_attrs, cpu_set_t &cpus, int speed, int samplings );
void printFrequencyTable ( map<int, vector<TIME> > &speed_v_lapses, int samplings );

const double ITERATIONS = 1000000.0;

void *SimpleComplexity ( void *thread_arg ) {
    TIME t1, t2;

    vector<TIME> *times = ( vector<TIME> * ) thread_arg;

    GetTime ( t1 );

    double i;
    double res = 0.0;
    for ( i = 0.0; i < ITERATIONS; i += 1.0 ) {
        res += sqrt ( i );
    }

    GetTime ( t2 );
//    simple_times.push_back( t1 );
//    simple_times.push_back( t2 );
    times->push_back ( t1 );
    times->push_back ( t2 );
    pthread_exit ( NULL );
}

void *MediumComplexity ( void *thread_arg ) {
    TIME t1, t2;

    vector<TIME> *times = ( vector<TIME> * ) thread_arg;

    GetTime ( t1 );

    double i, res = 0.0;
    for ( i = 1.0; i <= ITERATIONS; i += 1.0 ) {
        res += log ( i );
    }

    GetTime ( t2 );

//    medium_times.push_back( t1 );
//    medium_times.push_back( t2 );
    times->push_back ( t1 );
    times->push_back ( t2 );

    pthread_exit ( NULL );
}

void *SlowComplexity ( void *thread_arg ) {

    double i, result = 0.0;
    TIME t1, t2;

    vector<TIME> *times = ( vector<TIME> * ) thread_arg;

    GetTime ( t1 );

    for ( i = 0.0; i < ITERATIONS; i += 1.0 ) {
        result += sin ( i ) + cos ( i );
    }

    GetTime ( t2 );
//
//    slow_times.push_back( t1 );
//    slow_times.push_back( t2 );
    times->push_back ( t1 );
    times->push_back ( t2 );

    pthread_exit ( NULL );
}

void setCPURandomAvailSpeed ( int cpu_idx, map<int, string> &cpu_avail_freq ) {
    int j = 0, r2 = rand() % cpu_avail_freq.size();
    string err;

    for ( map<int, string>::iterator it = cpu_avail_freq.begin(); it != cpu_avail_freq.end() && j <= r2; ++j, ++it ) {
        if ( j == r2 ) {
            if ( !setCPUThrottledSpeed ( cpu_idx, it->second, err ) ) {
                cout << "-- ERROR: " << err << endl;
            }
        }
    }
}

void SpeedVTimeTest ( map<int, string> &cpu_avail_freq, int cpu_id = 1, int samplings = 10 ) {
    map<int, vector<TIME> > speed_v_lapses_simple;
    map<int, vector<TIME> > speed_v_lapses_medium;
    map<int, vector<TIME> > speed_v_lapses_slow;

    cpu_set_t cpus;
    pthread_attr_t thread_attrs;
    string err;

    CPU_ZERO ( &cpus );
    CPU_SET ( cpu_id, &cpus );
    pthread_attr_init ( &thread_attrs );
    pthread_attr_setaffinity_np ( &thread_attrs, sizeof ( cpu_set_t ), &cpus );
    pthread_attr_setdetachstate ( &thread_attrs, PTHREAD_CREATE_JOINABLE );

    for ( map<int, string>::iterator it = cpu_avail_freq.begin(); it != cpu_avail_freq.end(); it++ ) {
        if ( setCPUThrottledSpeed ( cpu_id, it->second, err ) ) {
//            clock_getres(CLOCK_MONOTONIC, &res);
//            printf(TIME_PRINT, res.tv_sec, res.FRAC);
//            printf("\n");
            SpeedTest ( speed_v_lapses_simple, SimpleComplexity, thread_attrs, cpus, it->first, samplings );
            SpeedTest ( speed_v_lapses_medium, MediumComplexity, thread_attrs, cpus, it->first, samplings );
            SpeedTest ( speed_v_lapses_slow, SlowComplexity, thread_attrs, cpus, it->first, samplings );

        } else {
            cout << "Error setting CPU Freq: " << err << "\n";
        }
    }

    cout << "# BEGINNING OF SIMPLE DATA\n";
    printFrequencyTable ( speed_v_lapses_simple, samplings );
    cout << "# BEGINNING OF MEDIUM DATA\n";
    printFrequencyTable ( speed_v_lapses_medium, samplings );
    cout << "# BEGINNING OF SLOW DATA\n";
    printFrequencyTable ( speed_v_lapses_slow, samplings );

    speed_v_lapses_simple.clear();
    speed_v_lapses_medium.clear();
    speed_v_lapses_slow.clear();
}

void ThreadTimingTest ( map<int, string > &cpu_avail_freq, int samplings = 10, int num_threads = 1 ) {
    cpu_set_t cpus[num_threads];
    pthread_attr_t thread_attrs[num_threads];
    pthread_t threads[num_threads];
    string err;
    int rc, tmp_idx;
    void *status;

    map<int, vector<TIME> > start_v_end;
    map<int, vector<TIME> >::iterator sve_it;

    for ( int i = 0; i < num_threads; ++i ) {
        CPU_ZERO ( &cpus[i] );
        CPU_SET ( i, &cpus[i] );
        pthread_attr_init ( &thread_attrs[i] );
        pthread_attr_setaffinity_np ( &thread_attrs[i], sizeof ( cpu_set_t ), &cpus[i] );
        pthread_attr_setdetachstate ( &thread_attrs[i], PTHREAD_CREATE_JOINABLE );
        start_v_end.insert ( pair<int, vector<TIME> > ( i, vector<TIME>() ) );
    }

    vector<TIME> sample_times;
    TIME t;

    for ( int i = 0; i < samplings; ++i ) {
        GetTime ( t );
        sample_times.push_back ( t );

        for ( sve_it = start_v_end.begin(); sve_it != start_v_end.end(); sve_it++ ) {
            tmp_idx = sve_it->first;
            if ( (rc = pthread_create ( &threads[tmp_idx], &thread_attrs[tmp_idx], SimpleComplexity, ( void * ) &sve_it->second ) )) {
                cout << "Error creating pthread: " << rc << "\n";
            }
        }

        for ( sve_it = start_v_end.begin(); sve_it != start_v_end.end(); sve_it++ ) {
            if ( (rc = pthread_join ( threads[sve_it->first], &status ) )) {
                cout << "Error joining thread: " << rc << "\n";
            }
        }

        GetTime ( t );
        sample_times.push_back ( t );
    }

    cout << "# BEGINNING OF PARALLEL THREAD TIMING TEST\n";
    printFrequencyTable ( start_v_end, samplings );

    cout << "Test #\tStart\tEnd\n";
    for ( int i = 0; i < samplings; ++i ) {
        cout << i << "\t";
        printf ( TIME_PRINT, sample_times[2 * i].tv_sec, sample_times[2 * i].FRAC );
        printf ( "\t" );
        printf ( TIME_PRINT, sample_times[2 * i + 1].tv_sec, sample_times[2 * i + 1].FRAC );
        printf ( "\n" );
    }

    cout << "#END OF TABLE\n";
}

void SpeedTest ( map<int, vector<TIME> > &speed_v_lapse, void * ( *algo ) ( void * ),
                 pthread_attr_t &thread_attrs, cpu_set_t &cpus, int speed, int samplings ) {
    pthread_t thread;
    int rc;
    void *status;

    map<int, vector<TIME> >::iterator speed_it;

    for ( int i = 0; i < samplings; i++ ) {
        if ( ( speed_it = speed_v_lapse.find ( speed ) ) == speed_v_lapse.end() ) {
            speed_v_lapse.insert ( pair<int, vector<TIME> > ( speed, vector<TIME>() ) );
            speed_it = speed_v_lapse.find ( speed );
        }

        rc = pthread_create ( &thread, &thread_attrs, algo, ( void * ) &speed_it->second );
        if ( rc ) {
            cout << "Error creating pthread: " << rc << "\n";
        }

        rc = pthread_join ( thread, &status );
        if ( rc ) {
            cout << "ERROR: pthread_join returned - " << rc << "\n";
        }



//        speed_it->second.push_back( times[0] );
//        speed_it->second.push_back( times[1] );
//        times.clear();
    }
}


void ThrottlingLagTest ( map<int, string> &cpu_avail_freq, int cpu_id = 1, int samplings = 10 ) {
    map<int, vector<TIME> > throt_times;

    TIME t1, t2;
    int r1;

    throt_times[0] = vector<TIME>();
    string err, speed;

    for ( int i = 0; i < samplings; i++ ) {
        r1 = rand() % cpu_avail_freq.size();

        for ( map<int, string>::iterator speed_it = cpu_avail_freq.begin(); speed_it != cpu_avail_freq.end(); speed_it++, r1-- ) {
            if ( r1 == 0 ) {
                speed = speed_it->second;
                break;
            }
        }

        GetTime ( t1 );

        if ( setCPUThrottledSpeed ( cpu_id, speed, err ) ) {

            GetTime ( t2 );
            throt_times[0].push_back ( t1 );
            throt_times[0].push_back ( t2 );
        } else {
            cout << "Error throttling processor\n";
            i--;
        }
    }

    cout << "# Average Throttling lag time\n";
    printFrequencyTable ( throt_times, samplings );
}

int main ( int argc, char **argv ) {
    srand ( time ( NULL ) );

    if ( geteuid() !=  0 ) {
        cout << "Must be run as root" << endl;
        return -1;
    }

    string cpu_freqs, err;

    map<int, string> cpu_avail_freq;
    map<int, string> userspace_cpu;
//
//    boost::char_separator<char> sep( " \n" );
//    boost::tokenizer<boost::char_separator<char> >::iterator tok_iter;

    int cpu_count = determineCPUCount();

    cout << "The system has " << cpu_count << " available processors." << endl;

    initUserspace(cpu_count, userspace_cpu);

//    for( i = 0; i < cpu_count; ++i ) {
//        getAvailableThrottlingSpeeds( i, cpu_freqs, err );
//        boost::tokenizer<boost::char_separator<char> > tokens( cpu_freqs, sep );
//
//        for( tok_iter = tokens.begin(); tok_iter != tokens.end(); tok_iter++ ) {
//            cpu_avail_freq.insert( pair<int, string> ( boost::lexical_cast<int> ( *tok_iter ), *tok_iter ) );
//        }
//    }

    fillAvailableThrottlingSpeeds ( cpu_avail_freq, cpu_count );

    for ( map<int, string>::iterator it = cpu_avail_freq.begin(); it != cpu_avail_freq.end(); ++it ) {
        cout << "Available frequencies: " << it->second << endl;
    }

    SpeedVTimeTest ( cpu_avail_freq );
    ThrottlingLagTest ( cpu_avail_freq );
    ThreadTimingTest ( cpu_avail_freq, 10, cpu_count );

    resetCPUs(userspace_cpu);

    return 0;
}
