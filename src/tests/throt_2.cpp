#include <iostream>
#include <unistd.h>
#include <cpufreq.h>

#include "utils/cpufunc.h"

using namespace std;

int main(int argc, char **argv) {
    if(geteuid() != 0){
        cout << "Must be running as root." << endl;
        return -1;
    }

//    int cpu_count = determineCPUCount();

    struct cpufreq_policy *policy;

    if((policy = cpufreq_get_policy(0)) == NULL) {
        cout << "Error getting cpufreq policy" << endl;
        return -1;
    }

    cout << policy->governor << " " << policy->min << " " << policy->max << endl;

    return 0;
}
