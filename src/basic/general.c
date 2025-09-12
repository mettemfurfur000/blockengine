#include "include/general.h"

#include <stdlib.h>

u64 generate_uuid()
{
    u64 ret = 0;
    u8 *buh = (u8 *)&ret;

    for (u8 i = 0; i < 8; i++)
        buh[i] = rand();

    ret ^= rand();

    return ret;
}