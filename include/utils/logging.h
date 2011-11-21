#ifndef LOGGING_H_INCLUDED
#define LOGGING_H_INCLUDED

#include <map>
#include <vector>

#include "timing.h"

using namespace std;

void printFrequencyTable( map<int, vector<TIME> > &speed_v_lapses, int samplings );

void printFrequencyTable2( map<int, vector<TIME> > &speed_v_lapses,
                           map<int, vector<uint64_t> > &loop_counts, int samplings );

#endif // LOGGING_H_INCLUDED
