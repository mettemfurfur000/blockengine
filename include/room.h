#ifndef ROOM_H
#define ROOM_H 1

#include "general.h"
#include "layer.h"

struct layer;

typedef struct room
{
    // level *p_level;

    char *name;
    u32 id;

    u32 width;
    u32 height;
    u32 depth;

    struct layer *layers;
    // TODO: entities
} room;

// room room_create(level *p_level, char *name, u32 width, u32 height, u32 depth);

#endif