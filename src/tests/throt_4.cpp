#include <iostream>
#include <vector>
#include <cstdlib>
#include <time.h>
#include <cstdio>
#include <pthread.h>
#include <map>
#include <unistd.h>

#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>

#include "utils/timing.h"
#include "utils/cpufunc.h"
#include "utils/procbind.h"
#include "utils/logging.h"

using namespace std;
namespace po = boost::program_options;

const string HELP_KEY = "help";
const string VERSION_KEY = "version";
const string THROTTLING_KEY = "throttle";
const string CPU_RUN_BIND_KEY = "running-proc-bind";
const string CPU_CHILD_BIND_KEY = "child-bind";
const string CPU_CUR_BIND_KEY = "cur-bind";
const string SKIP_RUN_BINDING_KEY = "skip-run-bind";
const string SKIP_ONDEMAND_KEY = "skip-ondemand";
const string TEST_SINGLE_EVENT_KEY = "single-event";
const string SAMPLING_KEY = "samplings";
const string FREQUENCY_EVENTS_KEY = "freq-events";
const string THREADS_PER_CORE_KEY = "thread-count";

struct node_t {
    char c;
    node_t *next, *prev;
    node_t ( char _c = 0 ) : c( _c ) {}
};

struct throt_thread {
    node_t *root;
    vector<char> moves;
    vector<TIME> times;
    uint64_t move_counts;

    throt_thread() : move_counts( 0 ) {}
};

struct ctrl_event_t {
//    uint64_t evt_threshold_count;
    string throt_speed;
    int loop_sec_offset;
};

struct throt_ctrl_t {
    node_t *root;
    int cpu_id;
    int node_count;
    vector<TIME> times;
    vector<uint64_t> counts;
    vector<ctrl_event_t> events;
};

int buildEvents(vector<string> &freq_event, int evt_idx, map<int, string> &avail_freq, vector<ctrl_event_t> &events);

bool parseArguments( int argc, char **argv, po::variables_map &vm ) {
    po::options_description general( "General Options" );
    general.add_options()
    (( HELP_KEY + ",h" ).c_str(), "Help options" )
    (( VERSION_KEY + ",v" ).c_str(), "Version" )
    ;

    po::options_description cpus( "CPU Options" );
    cpus.add_options()
    (( CPU_RUN_BIND_KEY + ",p" ).c_str(), po::value< vector<int> >( )->default_value( vector<int>( 1, 0 ), "0" )->multitoken(), "List of available CPUs to which all currently running processes should be bound." )
    (( CPU_CHILD_BIND_KEY + ",c" ).c_str(), po::value< vector<int> >()->default_value( vector<int>( 1, -1 ), "-1" )->multitoken(), "List of available CPUs to which all child threads/processes should be bound." )
    (( CPU_CUR_BIND_KEY + ",b" ).c_str(), po::value< vector<int> >()->default_value( vector<int>( 1, 0 ), "0" )->multitoken(), "List of available CPUs to which the parent process can be bound." )
    (( THREADS_PER_CORE_KEY + ",T").c_str(), po::value< int >()->default_value(1), "Threads spawned per core")
    ( SKIP_RUN_BINDING_KEY.c_str(), "Should skip building of previously running processes to specific CPU" )
    ( SKIP_ONDEMAND_KEY.c_str(), "After execution, leave CPUs in USERSPACE mode" )
    ;

    po::options_description tests( "Test Options" );
    tests.add_options()
    (( THROTTLING_KEY + ",t" ).c_str(), "Test throttling change rate" )
    (( FREQUENCY_EVENTS_KEY + ",f" ).c_str(), po::value< vector<string> >()->default_value( vector<string>(1, "MIN,MIN"), "MIN,MIN")->multitoken(), "Test algorithmic frequency throttled run time" )
    (( TEST_SINGLE_EVENT_KEY ).c_str(), "Non-thread spawning Event Test" )
    (( SAMPLING_KEY + ",s" ).c_str(), po::value<int>()->default_value( 10 ), "Specifies how many samples should be run" )
    ;

    po::options_description cmdline;
    cmdline.add( general ).add( cpus ).add( tests );

    po::positional_options_description p;
    p.add( FREQUENCY_EVENTS_KEY.c_str(), -1 );

    po::store( po::command_line_parser( argc, argv ).options( cmdline ).positional( p ).run(), vm );
    po::notify( vm );

    if( vm.count( HELP_KEY.c_str() ) ) {
        cout << cmdline << "\n";
        return false;
    }

    return true;
}

void generateCircularGraph( node_t *root, int node_count ) {
    node_t *cur = root;

    for( int i = 0; i < node_count - 1; ++i ) {
        cur->c = ( char )( rand() % 256 );
        cur->next = new node_t();
        cur->next->prev = cur;
        cur = cur->next;
    }

    cur->c = ( char )( rand() % 256 );
    cur->next = root;
    root->prev = cur;
}

void releaseCircularGraph( node_t *root ) {
    node_t *cur = root;

    cur->prev->next = NULL;
    cur->prev = NULL;

    while( cur->next != NULL ) {
        cur = cur->next;
        delete cur->prev;
    }
    delete cur;
}

void fillMoveList( vector<char> &moves, int move_count ) {
    for( int i = 0; i < move_count; ++i ) {
        moves.push_back(( char )( rand() % 2 ) );
    }
}

void timeTest( node_t *root, vector<char> &moves, vector<TIME> * times ) {
    node_t *cur = root;
    TIME t1, t2, t3;
    GetTime( t1 );

    for( vector<char>::iterator it = moves.begin(); it != moves.end(); it++ ) {
        if( *it ) {
            cur = cur->next;
        } else {
            cur = cur->prev->prev;
        }
    }

    GetTime( t2 );

    times->push_back( t1 );
    times->push_back( t2 );
}

void timeTest2( node_t *root, vector<TIME> * times, uint64_t &move_counts ) {

    node_t *cur = root;

    TIME t1, t2, tmp;
    GetTime( t1 );

    t2.tv_sec = t1.tv_sec + 5;

    move_counts = 0;

    do {
        if( rand() % 2 ) {
            cur = cur->next;
        } else {
            cur = cur->prev->prev;
        }

        move_counts++;

        GetTime( tmp );
    } while( tmp.tv_sec < t2.tv_sec );

    times->push_back( t1 );
    times->push_back( tmp );
}

void *threadableTimeTest( void *args ) {
    throt_thread *throt = ( throt_thread * ) args;

    timeTest( throt->root, throt->moves, &throt->times );

    pthread_exit( NULL );
}

void *threadableTimeTest2( void *args ) {
    throt_thread *throt = ( throt_thread * ) args;

    timeTest2( throt->root, &throt->times, throt->move_counts );

    pthread_exit( NULL );
}

bool checkGraphIsCircular( node_t *root, int expected_count ) {
    node_t *cur = root;
    int i = 0;

    do {
        i++;
        cur = cur->next;
    } while( cur != root || i > expected_count + 10 );

    if( i == expected_count ) {
        cout << "Forward Circular ... Confirmed" << endl;
    } else {
        cout << "Forward Circular ... " << i << " > " << expected_count << endl;
        return false;
    }

    i = 0;
    do {
        i++;
        cur = cur->prev;
    } while( cur != root || i > expected_count + 10 );

    if( i == expected_count ) {
        cout << "Reverse Circular ... Confirmed" << endl;
    } else {
        cout << "Reverse Circular ... " << i << " > " << expected_count << endl;
        return false;
    }

    return true;
}

void BasicTest() {
    node_t *root, * cur;

    int node_count = 1000;

    root = new node_t();

    generateCircularGraph( root, node_count );

    if( !checkGraphIsCircular( root, node_count ) ) {
        return;
    }

    vector<char> moves;
    vector<TIME> times;

    int move_count = 10000;

    TIME t3;

    for( ; move_count <= 10000000; move_count *= 10 ) {
        moves.clear();
        times.clear();

        fillMoveList( moves, move_count );

        timeTest( root, moves, &times );

        diff_TIME( t3, times[1], times[0] );

        printf( "Move Count: %d\n", (int)moves.size() );
//        printf( TIME_PRINT, times[0].tv_sec, times[0].FRAC );
        PrintTime(times[0]);
        printf( "\t" );
//        printf( TIME_PRINT, times[1].tv_sec, times[1].FRAC );
        PrintTime(times[1]);
        printf( "\t" );
//        printf( TIME_PRINT, t3.tv_sec, t3.FRAC );
        PrintTime(t3);
        printf( "\n" );
    }

    releaseCircularGraph( root );
}

void TestThrottledThreads( int cpu_id, int samplings ) {
    pthread_t thread;
    int rc;
    void *status;

    string err;

    int node_count = 1000;
    int move_count = 10000000;
    map<int, string> cpu_avail_freq;
    map<int, vector<TIME> > cpu_freq_times;
    map<int, vector<TIME> >::iterator cpu_freq_it;

    int cpu_count = determineCPUCount();

    cout << "Determining Available Speeds" << endl;

    fillAvailableThrottlingSpeeds( cpu_avail_freq, 1 );

    cout << "There are " << cpu_avail_freq.size() << " speeds available." << endl;

    cpu_set_t cpus;
    pthread_attr_t thread_attrs;

    CPU_ZERO( &cpus );
    CPU_SET( cpu_id, &cpus );
    pthread_attr_init( &thread_attrs );
    pthread_attr_setaffinity_np( &thread_attrs, sizeof( cpu_set_t ), &cpus );
    pthread_attr_setdetachstate( &thread_attrs, PTHREAD_CREATE_JOINABLE );

    throt_thread t_args;
    t_args.root = new node_t();

    generateCircularGraph( t_args.root, node_count );

    cout << "Built Circular Graph" << endl;

    for( int i = 0; i < samplings; i++ ) {
        cout << "Sampling ... " << i << endl;
        // setup thread arguments
        t_args.moves.clear();
        fillMoveList( t_args.moves, move_count );

        for( map<int, string>::iterator freq_it = cpu_avail_freq.begin(); freq_it != cpu_avail_freq.end(); freq_it++ ) {
            if(( cpu_freq_it = cpu_freq_times.find( freq_it->first ) ) == cpu_freq_times.end() ) {
                cpu_freq_times.insert( pair<int, vector<TIME> > ( freq_it->first, vector<TIME>() ) );
                cpu_freq_it = cpu_freq_times.find( freq_it->first );
            }

            if( setCPUThrottledSpeed( cpu_id, freq_it->second, err ) ) {
//                cout << "Frequency set" << endl;
                if( pthread_create( &thread, &thread_attrs, threadableTimeTest, ( void * ) &t_args ) ) {
                    cout << "Error creating thread\n";
                    break;
                }

                if( pthread_join( thread, &status ) ) {
                    cout << "Error joining thread\n";
                    break;
                }

                cpu_freq_it->second.push_back( t_args.times[0] );
                cpu_freq_it->second.push_back( t_args.times[1] );

                t_args.times.clear();
            } else {
                cout << "Unable to Throttle CPU\n";
            }
        }
    }

    printFrequencyTable( cpu_freq_times, samplings );
    cpu_freq_times.clear();
    releaseCircularGraph( t_args.root );
}

void TestThrottledThreads2( int cpu_id, int samplings ) {
    pthread_t thread;
    int rc;
    void *status;

    string err;

    int node_count = 1000;
    map<int, string> cpu_avail_freq;
    map<int, vector<uint64_t> > cpu_freq_loop_counts;
    map<int, vector<uint64_t> >::iterator cpu_lcnt_it;
    map<int, vector<TIME> > cpu_freq_times;
    map<int, vector<TIME> >::iterator cpu_freq_it;

//    int cpu_count = determineCPUCount();

    cout << "Determining Available Speeds" << endl;

    fillAvailableThrottlingSpeeds( cpu_avail_freq, 1 );

    cout << "There are " << cpu_avail_freq.size() << " speeds available." << endl;

    cpu_set_t cpus;
    pthread_attr_t thread_attrs;

    CPU_ZERO( &cpus );
    CPU_SET( cpu_id, &cpus );
    pthread_attr_init( &thread_attrs );
    pthread_attr_setaffinity_np( &thread_attrs, sizeof( cpu_set_t ), &cpus );
    pthread_attr_setdetachstate( &thread_attrs, PTHREAD_CREATE_JOINABLE );

    throt_thread t_args;
    t_args.root = new node_t();

    generateCircularGraph( t_args.root, node_count );

    cout << "Built Circular Graph" << endl;

    for( int i = 0; i < samplings; i++ ) {
        cout << "Sampling ... " << i << endl;
        // setup thread arguments
        for( map<int, string>::iterator freq_it = cpu_avail_freq.begin(); freq_it != cpu_avail_freq.end(); freq_it++ ) {
            if(( cpu_freq_it = cpu_freq_times.find( freq_it->first ) ) == cpu_freq_times.end() ) {
                cpu_freq_times.insert( pair<int, vector<TIME> > ( freq_it->first, vector<TIME>() ) );
                cpu_freq_it = cpu_freq_times.find( freq_it->first );
            }

            if(( cpu_lcnt_it = cpu_freq_loop_counts.find( freq_it->first ) ) == cpu_freq_loop_counts.end() ) {
                cpu_freq_loop_counts.insert( pair<int, vector<uint64_t> > ( freq_it->first, vector<uint64_t>() ) );
                cpu_lcnt_it = cpu_freq_loop_counts.find( freq_it->first );
            }

            if( setCPUThrottledSpeed( cpu_id, freq_it->second, err ) ) {
                if( pthread_create( &thread, &thread_attrs, threadableTimeTest2, ( void * ) &t_args ) ) {
                    cout << "Error creating thread\n";
                    break;
                }

                if( pthread_join( thread, &status ) ) {
                    cout << "Error joining thread\n";
                    break;
                }

                cpu_freq_it->second.push_back( t_args.times[0] );
                cpu_freq_it->second.push_back( t_args.times[1] );

                cpu_lcnt_it->second.push_back( t_args.move_counts );

                t_args.times.clear();
            } else {
                cout << "Unable to Throttle CPU\n";
            }
        }
    }

    printFrequencyTable2( cpu_freq_times, cpu_freq_loop_counts, samplings );
    cpu_freq_loop_counts.clear();
    cpu_freq_times.clear();
    releaseCircularGraph( t_args.root );
}

void EventBasedTest(throt_ctrl_t *ctrl) {
    vector<ctrl_event_t>::iterator evt_it;

    uint64_t cnt;
    string err;

    node_t *root = new node_t();
//    node_t *cur = ctrl->root;

    TIME t1, stop;
    GetTime( t1 );
    ctrl->times.push_back( t1 );

//    throts[idx].root = new node_t();
    generateCircularGraph( root, ctrl->node_count );
    node_t *cur = root;

    for( evt_it = ctrl->events.begin(); evt_it != ctrl->events.end(); evt_it++ ) {

        GetTime( t1 );
        ctrl->times.push_back( t1 );

        if( !setCPUThrottledSpeed( ctrl->cpu_id, evt_it->throt_speed, err ) ) {
            printf( "Unalbe to throttle CPU %d\n", ctrl->cpu_id );
            break;
        }

        GetTime( t1 );
        ctrl->times.push_back( t1 );

        GetTime( t1 );
        ctrl->times.push_back( t1 );

        stop.tv_sec = t1.tv_sec + evt_it->loop_sec_offset;
        stop.FRAC = t1.FRAC;

        cnt = 0;
        do {
            if( rand() % 2 ) {
                cur = cur->next;
            } else {
                cur = cur->prev->prev;
            }
            GetTime( t1 );
            cnt++;
        } while( t1.tv_sec < stop.tv_sec );

        GetTime( t1 );
        ctrl->times.push_back( t1 );
        ctrl->counts.push_back( cnt );
    }

    GetTime( t1 );
    ctrl->times.push_back( t1 );

    releaseCircularGraph(root);
}

void *EventThreads( void *args ) {
    throt_ctrl_t *ctrl = ( throt_ctrl_t * ) args;

    EventBasedTest( ctrl );

    pthread_exit( NULL );
}

void printParallelThreadsTable( map<int, string> &userspace_cpu, throt_ctrl_t *throts, int samplings, int thread_count ) {
    vector<TIME>::iterator times_it;
    vector<uint64_t>::iterator cnt_it;
    vector<ctrl_event_t>::iterator events_it;
    map<int, string>::iterator cpu_it;

    TIME t1, t2, diff;
    int i, j, k, l, t_idx;
    int throt_idx, thd_idx;

//    int event_count = (( throts[0].times.size() / samplings ) - 2 ) / 4;
    int event_count = throts[0].events.size();
    int time_offset = 2 + 4 * event_count;

    printf( "#\t\t\t\t" );
    for( i = 1; i <= event_count; ++i ) {
        printf( "Throttle #%d\t\t\tIteration #%d\t\t\t", i, i );
    }
    printf( "\n" );
    printf( "#Sample\tCPU ID\tThread ID\tBegin\t" );
    for( i = 0; i < event_count; ++i ) {
        printf( "Start (%s)\tFrequency\tEnd (%s)\tStart (%s)\tNodes Visited\tEnd (%s)\t", TIME_ABRV, TIME_ABRV, TIME_ABRV, TIME_ABRV );
    }
    printf( "Finish\n" );

    for( j = 0; j < samplings; j++ ) {
        printf( "%d", j );
        throt_idx = 0;
        for( cpu_it = userspace_cpu.begin(), i = 0; cpu_it != userspace_cpu.end(); cpu_it++, i++ ) {
            for(thd_idx = 0; thd_idx < thread_count; ++thd_idx, ++throt_idx) {
                printf( "\t%d", cpu_it->first );
                printf( "\t%d", throt_idx);
                printf( "\t" );
                k = j * time_offset;
    //            printf( TIME_PRINT, throts[i].times[k].tv_sec, throts[i].times[k].FRAC ); // begin
                PrintTime(throts[throt_idx].times[k]);
                printf( "\t" );

                ++k;
                for( l = 0; l < event_count; ++l ) {
                    t_idx = k + 4 * l;
                    t1 = throts[throt_idx].times[t_idx++];  // throttle start
                    t2 = throts[throt_idx].times[t_idx++];  // throttle end

//                    diff_TIME( diff, t2, t1 );
                    PrintTime(t1);
                    printf( "\t" );

                    printf( "%s\t", throts[throt_idx].events[l].throt_speed.c_str() );
    //                printf( TIME_PRINT, t1.tv_sec, t1.FRAC );

    //                printf( TIME_PRINT, t2.tv_sec, t2.FRAC );
                    PrintTime(t2);
                    printf( "\t" );
    //                printf( TIME_PRINT, diff.tv_sec, diff.FRAC );
//                    PrintTime(diff);
//                    printf( "\t" );

                    t1 = throts[throt_idx].times[t_idx++];  // iteration start
                    t2 = throts[throt_idx].times[t_idx++];  // iteration end

//                    diff_TIME( diff, t2, t1 );

    //                printf( TIME_PRINT, t1.tv_sec, t1.FRAC );
                    PrintTime(t1);
                    printf( "\t" );
    //                printf( TIME_PRINT, t2.tv_sec, t2.FRAC );

                    printf( "%lu\t", throts[throt_idx].counts[j * event_count + l] );

                    PrintTime(t2);
                    printf( "\t" );
    //                printf( TIME_PRINT, diff.tv_sec, diff.FRAC );
//                    PrintTime(diff);
//                    printf( "\t" );


                }
                k += time_offset - 2;
    //            printf( TIME_PRINT, throts[i].times[k].tv_sec, throts[i].times[k].FRAC ); // finish
                PrintTime(throts[throt_idx].times[k]);
                printf( "\n" );
            }
        }
    }
}

void TestParallelThreads( map<int, string> &userspace_cpu, vector<string> &freq_event, int samplings, int thread_count ) {
    int rc;
    void *status;

    string err;

    if( userspace_cpu.size() < 1 ) {
        printf( "Insufficient CPUs available for threading test\n" );
        return;
    }

    printf( "Using %d processors\n", (int)userspace_cpu.size() );

    int node_count = 1000;
    map<int, string> cpu_avail_freq;
    map<int, string>::iterator cpu_it;

    map<int, string>::iterator freq_it;

    int max_threads = thread_count * userspace_cpu.size();

    pthread_t threads[max_threads];
    cpu_set_t cpus[max_threads];
    cpu_set_t check_cpu;
    pthread_attr_t thread_attrs[max_threads];

    throt_ctrl_t throts[max_threads];

    int max_time_lapse = -1, tmp_lapse;

    printf("# filling available throttling speeds\n");
    fillAvailableThrottlingSpeeds( cpu_avail_freq, 1 );
    int i, j, idx;

    // initialize thread attributes with cpu affinity
    // initialize throttling controls
    idx = 0;

//    for(idx = 0; idx < max_threads; idx++) {
//        CPU_ZERO( &cpus[idx]);
//    }
//
//    for(cpu_it = userspace_cpu.begin(); cpu_it != userspace_cpu.end(); cpu_it++) {
//        for(idx = 0; idx < max_threads; idx++) {
//            CPU_SET( cpu_it->first, &cpus[idx]);
//        }
//    }

    idx = 0;
    for( cpu_it = userspace_cpu.begin(), i = 0; cpu_it != userspace_cpu.end(); cpu_it++, i++ ) {
        for(j = 0; j < thread_count; ++j, ++idx) {
            CPU_ZERO( &cpus[idx] );
            CPU_SET( cpu_it->first, &cpus[idx] );
            pthread_attr_init( &thread_attrs[idx] );
//            pthread_attr_setscope(&thread_attrs[idx], PTHREAD_SCOPE_SYSTEM);   // explicitly state the each thread is its own process; (END result - no effect from either scope; something must be overriding)

            int val = pthread_attr_setaffinity_np( &thread_attrs[idx], sizeof( cpu_set_t ), &cpus[idx] );

            if( val ) {
                printf( "Set Affinity Fail: %s\n", strerror(val) );
            }
            pthread_attr_setdetachstate( &thread_attrs[idx], PTHREAD_CREATE_DETACHED ); // make thread detachable
//            pthread_attr_setschedpolicy( &thread_attrs[idx], SCHED_RR);
//            throts[idx].root = new node_t();
//            generateCircularGraph( throts[idx].root, node_count );


            throts[idx].cpu_id = cpu_it->first;
            throts[idx].node_count = node_count;
    //        ctrl_event_t ctrl1, ctrl2;
    //
    //        freq_it = cpu_avail_freq.begin();
    //        ctrl1.throt_speed = freq_it->second;
    //        ctrl2.throt_speed = cpu_avail_freq.rbegin()->second;
    //
    //        if( cpu_it->first  % 4 ) {
    //            throts[i].events.push_back( ctrl1 );
    //            throts[i].events.push_back( ctrl1 );
    //        } else {
    //            throts[i].events.push_back( ctrl1 );
    //            throts[i].events.push_back( ctrl2 );
    //        }

            tmp_lapse = buildEvents(freq_event, idx, cpu_avail_freq, throts[idx].events);
            if(max_time_lapse < tmp_lapse) {
                max_time_lapse = tmp_lapse;
            }
        }
    }

    TIME t1;
    timespec t;

    for( int samp = 0; samp < samplings; ++samp ) {
        printf( "Sampling...%d\n", samp );
        idx = 0;
        for( cpu_it = userspace_cpu.begin(), i = 0; cpu_it != userspace_cpu.end(); cpu_it++, i++ ) {
            for(j = 0; j < thread_count; ++j, ++idx) {
                if( rc = pthread_create( &threads[idx], &thread_attrs[idx], EventThreads, ( void * ) &throts[idx] ) ) {
                    printf( "Error creating threads\n" );
                    return;
                }

//                if( rc = pthread_setaffinity_np( threads[idx], sizeof( cpu_set_t ), &cpus[idx] ) ) {
//                    printf( "Error setting affinity %d: %s\n", idx, strerror(rc) );
//                }
            }
        }

//        idx = 0;
//        for( i = 0; i < userspace_cpu.size(); ++i ) {
//            for(j = 0; j < thread_count; ++j, ++idx) {
////                printf("Checking %d of %d (%d * %d), %d, %d\n", idx, max_threads, thread_count, (int)userspace_cpu.size(), i, j);
//                if( rc = pthread_getaffinity_np( threads[idx], sizeof( cpu_set_t ), &check_cpu ) ) {
//                    printf( "Error checking affinity %d: %s\n", idx, strerror(rc) );
//                } else {
//                    for( cpu_it = userspace_cpu.begin(); cpu_it !=  userspace_cpu.end(); cpu_it++ ) {
//                        if( CPU_ISSET( cpu_it->first, &check_cpu ) ) {
//                            printf( "\t%d -> %d\n", idx, cpu_it->first );
//                        }
//                    }
//                }
//            }
//        }

//        idx = 0;
//        for( cpu_it = userspace_cpu.begin(), i = 0; cpu_it != userspace_cpu.end(); cpu_it++, i++ ) {
//            for(j = 0; j < thread_count; ++j, ++idx) {
//                if( rc = pthread_join( threads[idx], &status ) ) {
//                    printf( "Error joining threads\n" );
//                    return;
//                }
//            }
//        }



        GetTime(t1);

        t = convertTimeToTimespec(t1);
        t.tv_sec += (max_time_lapse + 5);

        pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

        pthread_cond_timedwait( &cond, &mutex, &t);
    }

    printParallelThreadsTable( userspace_cpu, throts, samplings, thread_count );

    for(idx = 0; idx < max_threads; idx++) {
        pthread_attr_destroy(&thread_attrs[idx]);
    }
}

void TestNoThreadEvent( map<int, string> &userspace_cpu, vector<string> &freq_event, int samplings ) {
    int node_count = 1000;
    map<int, string> cpu_avail_freq;
    map<int, string>::iterator cpu_it;

    map<int, string>::iterator freq_it;

    throt_ctrl_t throts;

    printf("Filling Available frequencies\n");
    fillAvailableThrottlingSpeeds( cpu_avail_freq, 1 );

    printf("Generating Circular graph\n");
//    throts.root = new node_t();
//    generateCircularGraph( throts.root, node_count );

    throts.cpu_id = userspace_cpu.begin()->first;
    throts.node_count = node_count;
    printf("Adding events to list\n");
    buildEvents(freq_event, 0, cpu_avail_freq, throts.events);

//    ctrl_event_t ctrl1, ctrl2;
//    freq_it = cpu_avail_freq.begin();
//
//    ctrl1.throt_speed = freq_it->second;
//    ctrl2.throt_speed = cpu_avail_freq.rbegin()->second;

//    if( throts.cpu_id % 4 ) {
//        throts.events.push_back( ctrl1 );
//        throts.events.push_back( ctrl1 );
//    } else {
//        throts.events.push_back( ctrl1 );
//        throts.events.push_back( ctrl2 );
//    }

    for(int samp = 0; samp < samplings; ++samp) {
        printf("Sampling %d\n", samp);
        EventBasedTest(&throts);
    }

    printParallelThreadsTable(userspace_cpu, &throts, samplings, 1);
}

int buildEvents(vector<string> &freq_event, int evt_idx, map<int, string> &avail_freq, vector<ctrl_event_t> &events) {
    map<int, string>::iterator min, mid;
    map<int, string>::reverse_iterator max;

    min = avail_freq.begin();
    mid = avail_freq.begin();
    max = avail_freq.rbegin();

    int time_lapse = 0;

    for(int m = avail_freq.size() / 2; m > 0; m--, mid++) {    }

    printf("# Throttling Events %d: ", evt_idx);
    int i = 0;
    vector<string>::iterator v_it = freq_event.begin();
    for(i = 0; i < evt_idx && i < freq_event.size() - 1; i++, v_it++) {
    }

//    for( v_it != freq_event.end(); v_it++) {

    boost::char_separator<char> sep(",");

    boost::tokenizer<boost::char_separator<char> >::iterator tok_iter, tok_iter2;

    boost::tokenizer<boost::char_separator<char> > tokens( *v_it, sep );

    for(tok_iter = tokens.begin(); tok_iter != tokens.end(); tok_iter++) {
        ctrl_event_t evt;
        if(boost::algorithm::istarts_with(*tok_iter, "min") ) {
            printf("MIN for ");
            evt.throt_speed = min->second;
        } else if(boost::algorithm::istarts_with(*tok_iter, "mid") ){
            printf("MID for ");
            evt.throt_speed = mid->second;
        } else if(boost::algorithm::istarts_with(*tok_iter, "max") ){
            printf("MAX for ");
            evt.throt_speed = max->second;
        } else {
            printf("Skipping Requested Frequency: %s\n", tok_iter->c_str());
            continue;
        }

        if(boost::algorithm::contains(*tok_iter, ":")) {
            evt.loop_sec_offset = 0;
            sscanf(tok_iter->c_str(), "%*[^:]:%d", &(evt.loop_sec_offset));

        } else {
            evt.loop_sec_offset = 5;
        }
        time_lapse += evt.loop_sec_offset;
        printf( "%d sec -> ", evt.loop_sec_offset);
        events.push_back(evt);
    }
    printf("0\n");
    return time_lapse;
}

int main( int argc, char **argv ) {
    if( geteuid() !=  0 ) {
        cout << "Must be run as root" << endl;
        return -1;
    }

    po::variables_map vm;
    if( !parseArguments( argc, argv, vm ) ) {
        return 1;
    }

    srand( time( NULL ) );

    map<int, string> userspace_cpu;
    map<int, string> avail_cpu;

    int cpu_count = determineCPUCount();
    string err;

    map<int, cpu_set_t> proc_list;
    vector<int> v;
    cpu_set_t proc_bind_mask, cur_bind_mask;

    v = vm[CPU_RUN_BIND_KEY.c_str()].as< vector<int> >();
    buildMask( v, proc_bind_mask );

    v = vm[CPU_CUR_BIND_KEY.c_str()].as< vector<int> >();
    buildMask( v, cur_bind_mask );

    if( vm.count( SKIP_RUN_BINDING_KEY.c_str() ) == 0 ) {
        BindAllProcTo( proc_list, cpu_count, proc_bind_mask );
    }

    bindCurProcTo( cur_bind_mask );

    printCurrentProcBinding();

    initUserspace( cpu_count, userspace_cpu );

    pid_t cur_pid = getpid();
    int ret_val;

    struct sched_param sched_p;

    sched_p.sched_priority = 50;        // make sure this process is executing in real time
//
    if((ret_val = sched_setscheduler( cur_pid, SCHED_RR, &sched_p)) != 0) {
        printf("Unable to change Scheduler Policy to RR: %d\n", cur_pid);
    }

    v = vm[CPU_CHILD_BIND_KEY.c_str()].as< vector<int> >();

    sort( v.begin(), v.end() );
    int j = 0;
    for( map<int, string>::iterator user_it = userspace_cpu.begin(); user_it != userspace_cpu.end() && j < v.size(); user_it++ ) {
        if( v[j] == -1 ) {
            avail_cpu.insert( pair<int, string>( user_it->first, user_it->second ) );
        } else if( v[j] == user_it->first ) {
            printf( "CPU%d is available for use by parallel threads\n", user_it->first );
            avail_cpu.insert( pair<int, string>( user_it->first, user_it->second ) );
            ++j;
        }
    }

    int samplings = vm[SAMPLING_KEY.c_str()].as<int>();
    int thread_count = vm[THREADS_PER_CORE_KEY.c_str()].as<int>();

//    BasicTest();
//    TestThrottledThreads( avail_cpu.begin()->first, samplings );
//    TestThrottledThreads2( avail_cpu.begin()->first, samplings );

    vector<string> freq_events = vm[FREQUENCY_EVENTS_KEY.c_str()].as< vector<string> >();

    if( vm.count(TEST_SINGLE_EVENT_KEY) ) {
        if(avail_cpu.size() > 1) {
            map<int, string>::iterator avail_it = avail_cpu.begin();
            avail_it++;
            avail_cpu.erase(avail_it, avail_cpu.end());

            vector<int> tmp;
            tmp.push_back(avail_cpu.begin()->first);
            buildMask(v, cur_bind_mask);
            bindCurProcTo(cur_bind_mask);
        }
        printf("Testing Non-Threaded Events\n");
        TestNoThreadEvent(avail_cpu, freq_events, samplings);
    } else {
        TestParallelThreads( avail_cpu, freq_events, samplings, thread_count );
    }

    resetAffinities( proc_list );

    if(vm.count(SKIP_ONDEMAND_KEY.c_str()) == 0)
        resetCPUs( userspace_cpu );


    userspace_cpu.clear();
    avail_cpu.clear();
    proc_list.clear();

    return 0;
}
