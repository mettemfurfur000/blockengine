#ifndef ROOM_H
#define ROOM_H 1

#include "general.h"
#include "layer.h"
#include "level.h"

typedef struct room
{
    level *parent;
    u32 id;
    char *name;

    u32 width;
    u32 height;
    u32 depth;

    layer *layers;
    // TODO: entities
} room;

#endif