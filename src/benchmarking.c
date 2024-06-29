#include "include/benchmarking.h"

// returns ticks, have to be passed in bench_end to get actual time
int bench_start()
{
    return clock();
}

// returns time in seconds
float bench_end(int bench_start)
{
    return ((float)(clock() - bench_start)) / CLOCKS_PER_SEC;
}
