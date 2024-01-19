#ifndef BENCHMARKING
#define BENCHMARKING

#include <time.h>

int bench_start()
{
    return clock();
}

float bench_end(int bench_start)
{
    return ((float)(clock() - bench_start)) / CLOCKS_PER_SEC;
}

#endif