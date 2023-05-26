#include <cstdio>
#include "memory_control_functions.c"

const int bigendianchecker = 1;
bool is_bigendian() { return (*(char*)&bigendianchecker) == 0;};

int main()
{
    return 0;
}