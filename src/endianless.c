#ifndef ENDIANLESS_H
#define ENDIANLESS_H
#include "game_types.h"

int is_bigendian()
{
    unsigned int x = 1;
    char *c = (char *)&x;
    return (int)*c;
}

int swap_order_i(int x)
{
    int b0, b1, b2, b3;

    b0 = (x & 0x000000ff) << 24u;
    b1 = (x & 0x0000ff00) << 8u;
    b2 = (x & 0x00ff0000) >> 8u;
    b3 = (x & 0xff000000) >> 24u;

    return b0 | b1 | b2 | b3;
}

short swap_order_s(short x)
{
    short b0, b1;

    b0 = (x & 0x00ff) << 8u;
    b1 = (x & 0xff00) >> 8u;

    return b0 | b1;
}

int make_normal_i(int x)
{
    return is_bigendian() ? swap_order_i(x) : x;
}

int make_normal_s(int x)
{
    return is_bigendian() ? swap_order_s(x) : x;
}

#endif