#ifndef PROC_BIND_H_INCLUDED
#define PROC_BIND_H_INCLUDED

#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include <sched.h>
#include <errno.h>
#include <dirent.h>
#include <cstring>
#include <unistd.h>

using namespace std;

void buildMask( vector<int> &v, cpu_set_t &mask );

bool BindAllProcTo( map<int, cpu_set_t> &proc_aff, int cpu_count, cpu_set_t &bind_mask );
void bindCurProcTo( cpu_set_t &mask );

void printCurrentProcBinding();

void resetAffinities( map<int, cpu_set_t> &proc_aff );

#endif // PROC_BIND_H_INCLUDED
