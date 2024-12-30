#ifndef LEVEL_H
#define LEVEL_H 1

#include "general.h"

typedef struct level
{
    // TODO: add block registries
    void *rooms;
    char *name;

    u32 num_rooms;
} level;

#endif