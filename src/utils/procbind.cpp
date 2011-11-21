#include "utils/procbind.h"

bool BindAllProcTo( map<int, cpu_set_t> &proc_aff, int cpu_count, cpu_set_t &bind_mask ) {
    struct dirent **namelist;
    int n = scandir( "/proc", &namelist, NULL, NULL );
    int pid;

    cpu_set_t mask;
//    cpu_set_t bind_mask;
//
//    CPU_ZERO(&bind_mask);
//    CPU_SET_S( cpu_id, sizeof( cpu_set_t ), &bind_mask );

    if( n < 0 ) {
        printf( "Error scanning directory for files\n" );
        return false;
    }

    while( n-- ) {
        if( namelist[n]->d_type == DT_DIR && ( pid = atoi( namelist[n]->d_name ) ) != 0 ) {
            if( sched_getaffinity(( pid_t )pid, sizeof( cpu_set_t ), &mask ) == 0 ) {
//                printf( "Found: %s -> d ", namelist[n]->d_name );
//
//                for( int i = 0; i < cpu_count; i++ ) {
//                    if( CPU_ISSET_S( i, sizeof( cpu_set_t ), &mask ) ) {
//                        printf( "%d,", i );
//                    }
//                }
//
//                printf( "\t" );

                if( sched_setaffinity(( pid_t )pid, sizeof( cpu_set_t ), &bind_mask ) == 0 ) {
                    proc_aff.insert( pair<int, cpu_set_t>( pid, mask ) );

//                    if( sched_getaffinity(( pid_t )pid, sizeof( cpu_set_t ), &mask ) == 0 ) {
//                        for( int i = 0; i < cpu_count; i++ ) {
//                            if( CPU_ISSET_S( i, sizeof( cpu_set_t ), &mask ) ) {
//                                printf( "%d,", i );
//                            }
//                        }
//                    } else {
//                        printf("\nError getting affinity: %s\n", strerror(errno));
//                    }
                } else {
//                    printf( "Error setting affinity: %d %d -> %s\n", pid, errno, strerror( errno ) );
                    printf("Error setting affinity: %d\n", pid);
                }

            } else {
                printf("Error getting affinity: %d\n", pid);
            }
        }
    }

    return true;
}

void resetAffinities( map<int, cpu_set_t> &proc_aff ) {
    for( map<int, cpu_set_t>::iterator proc_it = proc_aff.begin(); proc_it != proc_aff.end(); proc_it++ ) {
        if( sched_setaffinity(( pid_t )proc_it->first, sizeof( cpu_set_t ), &proc_it->second ) != 0 ) {
            printf( "Error resetting Process Affinity: %d %d -> %s\n", proc_it->first, errno, strerror( errno ) );
        }
    }
}

void buildMask( vector<int> &v, cpu_set_t &mask ) {
    CPU_ZERO( &mask );
    for( vector<int>::iterator v_it = v.begin(); v_it != v.end(); v_it++ ) {
        CPU_SET_S( *v_it, sizeof( cpu_set_t ), &mask );
    }
}

void bindCurProcTo( cpu_set_t &mask ) {
    int pid = getpid();
    if( sched_setaffinity(( pid_t )pid, sizeof( cpu_set_t ), &mask ) ) {
        printf( "Error setting current process affinity: %d %d -> %s\n", pid, errno, strerror( errno ) );
    }
}

void printCurrentProcBinding() {
    pid_t pid = getpid();
    cpu_set_t mask;

    if( sched_getaffinity( pid, sizeof( cpu_set_t ), &mask )  == 0 ) {
        printf("Current Process %d -> ", pid);
        for( int i = 0, j = 0; j < CPU_COUNT_S( sizeof(cpu_set_t), &mask); i++ ) {
            if( CPU_ISSET_S( i, sizeof( cpu_set_t ), &mask ) ) {
                printf( "%d,", i );
                ++j;
            }
        }
        printf("\n");
    } else {
        printf( "Error getting affinity: %d %d -> %s\n", pid, errno, strerror( errno ) );
    }
}

