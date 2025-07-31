#ifndef LEVEL_H
#define LEVEL_H 1

#include "../vec/src/vec.h"
#include "block_registry.h"
#include "general.h"
#include "hashtable.h"

#define LAYER_FLAG_HAS_VARS 0b00000001
#define LAYER_FLAG_HAS_REGISTRY 0b00000010

typedef struct var_holder
{
    blob *b_ptr; // points to a valid instance of a record with vars
    u8 active;  // will be marked as inactive if freed
} var_holder;

typedef vec_t(var_holder) var_holder_vec_t;

typedef struct var_object_pool
{
    var_holder_vec_t vars;
    u16 inactive_count;
    u16 inactive_limit;
} var_object_pool;

typedef struct layer
{
    u64 uuid;
    void *parent_room;
    // hash_node **vars;         // hashtable for block vars - variables, unique
    // for said block. constant values are stored in the block registry, not
    // here

    // hashtables are way too slow for this, using an array instead

    var_object_pool var_pool;
    block_registry *registry; // block registry used by this layer. if NULL, the
                              // layer acts as a simple array of ids
    u8 *blocks;               // array of blocks, each of size bytes_per_block

    u32 width;                //
    u32 height;               //
    u8 block_size;            // bytes per block id
    u8 var_index_size;        // bytes per var index - a special index for the vars
                              // array
    u8 total_bytes_per_block; // sum of block_size and var_index_size

    u8 flags; //
} layer;

typedef struct room
{
    u64 uuid;
    char *name;
    void *parent_level;

    u32 width;  //
    u32 height; //

    vec_void_t layers;
    // TODO: implement support for entities

    u8 flags;
} room;

typedef struct level
{
    u64 uuid;
    vec_void_t registries;
    vec_void_t rooms;

    char *name;
    u8 flags;
} level;

#define BLOCK_ID_PTR(l, x, y) (l->blocks + ((y * l->width) + x) * l->total_bytes_per_block)
#define MERGE32_TO_64(a, b) (((u64)a << 32) | (u64)b)

u8 block_set_id(layer *l, u32 x, u32 y, u64 id);
u8 block_get_id(layer *l, u32 x, u32 y, u64 *id);
u8 block_move(layer *l, u32 x, u32 y, u32 dx, u32 dy);

// u8 block_get_vars(layer *l, u32 x, u32 y, blob *vars_out);
u8 block_get_vars(layer *l, u32 x, u32 y, blob **vars_out);

u64 block_get_vars_index(layer *l, u32 x, u32 y);
void block_var_index_set(layer *l, u32 x, u32 y, u64 index);

u8 block_delete_vars(layer *l, u32 x, u32 y);
u8 block_copy_vars(layer *l, u32 x, u32 y, blob vars);

u8 layer_clean_vars(layer *l);

u8 init_layer(layer *l,
              room *parent_room); // call after setting their resolutions
u8 init_room(room *r, level *parent_level);
u8 init_level(level *l);

u8 free_layer(layer *l);
u8 free_room(room *r);
u8 free_level(level *l);

level *level_create(const char *name);
room *room_create(level *parent, const char *name, u32 w, u32 h);
layer *layer_create(room *parent, block_registry *registry_ref, u8 bytes_per_block, u8 bytes_per_index, u8 flags);
// utils

// turns string into formatted block chain
// supposed to be used with a special character block
void bprintf(layer *l, const u64 character_block_id, u32 orig_x, u32 orig_y, u32 length_limit, const char *format, ...);

#endif