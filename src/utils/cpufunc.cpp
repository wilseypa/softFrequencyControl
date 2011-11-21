#include "utils/cpufunc.h"

void Test1() {
    FILE *fp;
    char *buffer = new char[BUFFER_SIZE];

    long lSize;

    fp = fopen ( ( CPU0_FILE_PATH + SCALING_GOVERNOR_FILE ).c_str(), "rw" );
    if ( fp == NULL ) {
        cerr << "Error opening Governor file." << endl;
    } else {
        cout << "Opened the file." << endl;
        fseek ( fp, 0, SEEK_END );
        lSize = ftell ( fp );
        fseek ( fp, 0, SEEK_SET );

        cout << "File Size: " << lSize << endl;

        int bytes_read = fread ( buffer, lSize, 1, fp );

        cout << "Read: " << bytes_read << endl;
        cout << buffer << endl;
    }
    fclose ( fp );
}

void Test2() {
    FILE *fp;
    char *buffer = new char[BUFFER_SIZE];

    long lSize;

    fp = fopen ( ( CPU0_FILE_PATH + SCALING_AVAILABLE_GOVERNOR ).c_str(), "r" );
    if ( fp == NULL ) {
        cout << "Error opening governor file" << endl;
    } else {
        cout << "Opened the file" << endl;
        fseek ( fp, 0, SEEK_END );
        lSize = ftell ( fp );
        fseek ( fp, 0, SEEK_SET );

        if ( lSize > BUFFER_SIZE ) {
            cout << "Buffer isn't large enough to read file. " << lSize << endl;
        } else {
            fread ( buffer, lSize, 1, fp );

            //bufferdump(buffer, lSize);

            if ( boost::algorithm::contains ( buffer, USERSPACE ) ) {
                cout << "Processor can be moved controlled from userspace" << endl;
            }
        }
    }
    fclose ( fp );
}

int determineCPUCount() {
    FILE *fp;
    char *res = new char[128];
    memset ( res, 0, sizeof ( res ) );
    fp = popen ( "/bin/cat /proc/cpuinfo | grep -c '^processor'", "r" );
    fread ( res, 1, sizeof ( res ) - 1, fp );
    fclose ( fp );

    for ( int i = 0; i < 128; ++i ) {
        if ( res[i] < '0' || res[i] > '9' ) {
            res[i] = ( char ) 0;
            break;
        }
    }

    return atoi ( res );
}

void bufferdump ( char *buffer, int buffer_size ) {
    for ( int i = 0, j = 0; i < buffer_size; ++i, ++j ) {
        if ( buffer[i] >= ' ' && buffer[i] <= '~' ) {
            cout << ( char ) buffer[i];
        } else {
            cout << ( int ) buffer[i];
        }
        cout << " ";
        if ( j >= 16 ) {
            cout << endl;
            j = 0;
        }
    }

    cout << endl;
}

bool checkCPU_Avail_Governor ( int cpu_idx, const string &governor, string &err ) {
    FILE *fp;
    char *buffer = new char[BUFFER_SIZE];
    char *file_path = new char[100];

    sprintf ( file_path, "%s%d%s%s", CPU_FILE.c_str(), cpu_idx, CPU_FREQ.c_str(), SCALING_AVAILABLE_GOVERNOR.c_str() );

    fp = fopen ( file_path, "r" );

    if ( fp == NULL ) {
        err = "Unable to open file.";
        return false;
    }

    fread ( buffer, BUFFER_SIZE, 1, fp );

    fclose ( fp );

    return boost::algorithm::contains ( buffer, governor );
}

bool checkCPU_Governor_Mode ( int cpu_idx, const string &governor, string &err ) {
    FILE *fp;
    char *buffer = new char[BUFFER_SIZE];
    char *file_path = new char[100];

    sprintf ( file_path, "%s%d%s%s", CPU_FILE.c_str(), cpu_idx, CPU_FREQ.c_str(), SCALING_GOVERNOR_FILE.c_str() );

    fp = fopen ( file_path, "r" );

    if ( fp == NULL ) {
        err = "Unable to open file";
        return false;
    }

    fread ( buffer, BUFFER_SIZE, 1, fp );

    fclose ( fp );

    return boost::algorithm::contains ( buffer, governor );
}

bool setCPU_Govenor_Mode ( int cpu_idx, const string &governor, string &err ) {
    FILE *fp;
    char *buffer = new char[BUFFER_SIZE];
    char *file_path = new char[100];

    sprintf ( file_path, "%s%d%s%s", CPU_FILE.c_str(), cpu_idx, CPU_FREQ.c_str(), SCALING_GOVERNOR_FILE.c_str() );

    fp = fopen ( file_path, "w" );

    if ( fp == NULL ) {
        err = "Unable to open file";
        return false;
    }

    int size = fwrite ( governor.c_str(), governor.length(), 1, fp );

    fclose ( fp );

    return size == 1;
}

bool getAvailableThrottlingSpeeds ( int cpu_idx, string &freqs, string &err ) {
    FILE *fp;
    char *freq_buffer = new char[BUFFER_SIZE];
    char *file_path = new char[100];

    sprintf ( file_path, "%s%d%s%s", CPU_FILE.c_str(), cpu_idx, CPU_FREQ.c_str(), SCALING_AVAILABLE_FREQ.c_str() );

    fp = fopen ( file_path, "r" );
    if ( fp == NULL ) {
        err = "Unable to open file";
        return false;
    }

    fread ( freq_buffer, BUFFER_SIZE, 1, fp );
    fclose ( fp );

    freqs.assign ( freq_buffer );

    //bufferdump(freq_buffer, BUFFER_SIZE);

    return true;
}

bool setCPUThrottledSpeed ( int cpu_idx, const string &speed, string &err ) {
    FILE *fp;

    char *buffer = new char[BUFFER_SIZE];
    char *file_path = new char[100];

    sprintf ( file_path, "%s%d%s%s", CPU_FILE.c_str(), cpu_idx, CPU_FREQ.c_str(), SCALING_SETSPEED_FILE.c_str() );

    fp = fopen ( file_path, "w" );

    if ( fp == NULL ) {
        err = "Unable to open Set Speed file";
        return false;
    }

    int size = fwrite ( speed.c_str(), speed.length(), 1, fp );
    fclose ( fp );

    sprintf ( file_path, "%s%d%s%s", CPU_FILE.c_str(), cpu_idx, CPU_FREQ.c_str(), SCALING_SETMAXSPEED_FILE.c_str() );

    fp = fopen ( file_path, "w" );

    if ( fp == NULL ) {
        err = "Unable to open Max Speed file";
        return false;
    }


    size = fwrite ( speed.c_str(), speed.length(), 1, fp );
    fclose ( fp );

    return size == 1;
}

bool fillAvailableThrottlingSpeeds( map<int, string> &cpu_avail_freq, int cpu_count) {
    string cpu_freqs, err;
    boost::char_separator<char> sep( " \n" );
    boost::tokenizer<boost::char_separator<char> >::iterator tok_iter;

    for( int i = 0; i < cpu_count; ++i ) {
        getAvailableThrottlingSpeeds( i, cpu_freqs, err );
        boost::tokenizer<boost::char_separator<char> > tokens( cpu_freqs, sep );

        for( tok_iter = tokens.begin(); tok_iter != tokens.end(); tok_iter++ ) {
            cpu_avail_freq.insert( pair<int, string> ( boost::lexical_cast<int> ( *tok_iter ), *tok_iter ) );
        }
    }
    return true;
}


bool initUserspace(int cpu_count, map<int, string> &orig_state){
    string governor, err;

    for ( int i = 0; i < cpu_count; i++ ) {
        if(checkCPU_Governor_Mode(i, governor, err)) {
            orig_state.insert( pair<int, string>(i, governor));
        }

        if ( !boost::algorithm::iequals(governor, USERSPACE) && ! setCPU_Govenor_Mode ( i, USERSPACE, err ) ) {
            //userspace_cpu.push_back ( i );
            orig_state.erase(i);
        }
    }

    return true;
}

bool resetCPUs(map<int, string> &orig_state){
    string err;

    for ( map<int,string>::iterator it = orig_state.begin(); it != orig_state.end(); ++it ) {
        cout << "Returning processor " << it->first << " to " << ONDEMAND << " mode ...";
        if ( setCPU_Govenor_Mode ( it->first, ONDEMAND, err ) ) {

            cout << " COMPLETE" << endl;
        } else {
            cout << " FAILED" << endl;
            cout << "-- ERROR: " << err << endl;
        }
    }

    return true;
}
