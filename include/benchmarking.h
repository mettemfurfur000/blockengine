#ifndef BENCHMARKING_H
#define BENCHMARKING_H 1

#include <time.h>

// returns ticks, have to be passed in bench_end to get actual time
int bench_start();
// returns time in seconds
float bench_end(int bench_start);

#endif