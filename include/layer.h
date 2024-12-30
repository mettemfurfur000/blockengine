#ifndef LAYER_H
#define LAYER_H 1

#include "general.h"
#include "hashtable.h"
#include "endianless.h"
#include "flags.h"
#include "room.h"

// #define LAYER_FLAG_HAS_TAGS 0x01

typedef struct layer
{
    room *p_room;
    hash_node **block_tag_table;

    u8 *blocks;
    u8 bytes_per_block;
    u8 flags;
} layer;

u8 set_id(layer *l, u32 x, u32 y, u64 id);
u8 get_id(layer *l, u32 x, u32 y, u64 *id);

u8 get_tags(layer *l, u32 x, u32 y, blob *tags_out);
u8 set_tags(layer *l, u32 x, u32 y, blob tags);

#endif