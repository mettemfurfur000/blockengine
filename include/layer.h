#ifndef LAYER_H
#define LAYER_H 1

#include "general.h"
#include "hashtable.h"
#include "structures.h"

typedef struct layer
{
    room *p_room;
    level *p_level;
    hash_node **block_tag_table;

    u8 *blocks;
    u8 bytes_per_block;
    u8 flags;
} layer;

#endif