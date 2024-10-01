#ifndef FLAGS_H
#define FLAGS_H

#include "engine_types.h"

#define FLAG_IS_VALID_POS(f, i) i >= 0 && i < sizeof(f) ? 1 : 0
#define FLAG_GET(f, i) f & (1 << i)

#define FLAG_FLIP(f, i) f ^= (1 << i);

#define FLAG_ON(f, i) f |= 1 << (i);
#define FLAG_OFF(f, i) f &= ~(1 << (i));

#define FLAG_SET(f, i, val)    \
    if (FLAG_GET(f, i) != val) \
    FLAG_FLIP(f, i)

void test()
{
    byte testflags;

    if (FLAG_IS_VALID_POS(testflags, 3))
    {
        FLAG_SET(testflags, 3, 1);
    }
}

#endif