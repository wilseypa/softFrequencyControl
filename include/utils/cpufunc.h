#ifndef CPUFUNC_H_
#define CPUFUNC_H_

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <map>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

const int BUFFER_SIZE = 4096;

const string CPU0_FILE_PATH = "/sys/devices/system/cpu/cpu0/cpufreq/";
const string CPU1_FILE_PATH = "/sys/devices/system/cpu/cpu1/cpufreq/";

const string CPU_FILE = "/sys/devices/system/cpu/cpu";
const string CPU_FREQ = "/cpufreq/";

const string SCALING_GOVERNOR_FILE = "scaling_governor";
const string SCALING_SETSPEED_FILE = "scaling_setspeed";
const string SCALING_SETMAXSPEED_FILE = "scaling_max_freq";
const string SCALING_AVAILABLE_GOVERNOR = "scaling_available_governors";
const string SCALING_AVAILABLE_FREQ = "scaling_available_frequencies";

const string USERSPACE = "userspace";
const string ONDEMAND = "ondemand";


void Test1() ;

void Test2() ;

int determineCPUCount();

void bufferdump ( char *buffer, int buffer_size );

bool initUserspace(int cpu_count, map<int, string> &orig_state);
bool resetCPUs(map<int, string> &orig_state);

bool checkCPU_Avail_Governor ( int cpu_idx, const string &governor, string &err );

bool checkCPU_Governor_Mode ( int cpu_idx, const string &governor, string &err );

bool setCPU_Govenor_Mode ( int cpu_idx, const string &governor, string &err ) ;

bool fillAvailableThrottlingSpeeds( map<int, string> &cpu_avail_freq, int cpu_count);

bool getAvailableThrottlingSpeeds ( int cpu_idx, string &freqs, string &err );
bool setCPUThrottledSpeed ( int cpu_idx, const string &speed, string &err ) ;

#endif // CPUFUNC_H_
