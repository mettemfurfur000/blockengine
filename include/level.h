#ifndef LEVEL_H
#define LEVEL_H 1

#include "hashtable.h"
#include "endianless.h"
#include "block_registry.h"
#include "flags.h"
#include "general.h"
#include "block_registry.h"
#include "../vec/src/vec.h"

#define LAYER_FLAG_HAS_vars 0x01
#define LAYER_FLAG_HAS_REGISTRY 0x02

typedef struct layer
{
    hash_node **vars;         // hashtable for block vars - variables, unique for said block. constant values are stored in the block registry, not here
    block_registry *registry; // block registry used by this layer. if NULL, the layer acts as a simple array of ids
    u8 *blocks;               // array of blocks, each of size bytes_per_block
    u8 bytes_per_block;       //
    u8 flags;                 //
} layer;

typedef vec_t(layer) layer_vec_t;

typedef struct room
{
    char *name;
    void *parent_level;

    u32 width;  //
    u32 height; //
    u32 depth;  // number of layers

    // each layer has the same width and height

    layer_vec_t layers;
    // TODO: implement support for entities
} room;

typedef vec_t(room) room_vec_t;

typedef struct level
{
    char *name;

    vec_registries_t registries;
    room_vec_t rooms;
} level;

u8 block_set_id(room *r, u32 layer_index, u32 x, u32 y, u64 id);
u8 block_get_id(room *r, u32 layer_index, u32 x, u32 y, u64 *id);

u8 block_get_vars(room *r, u32 layer_index, u32 x, u32 y, blob *vars_out);
u8 block_set_vars(room *r, u32 layer_index, u32 x, u32 y, blob vars);

u8 alloc_layer(layer *l, room *parent_room);
u8 free_layer(layer *l);
u8 alloc_room(room *r, level *parent_level, char *name, u32 width, u32 height, u32 depth);
u8 free_room(room *r);

#endif