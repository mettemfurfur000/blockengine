#ifndef LEVEL_H
#define LEVEL_H 1

#include "../vec/src/vec.h"
#include "block_registry.h"
#include "general.h"
#include "hashtable.h"
#include "handle.h"

#define LAYER_FLAG_HAS_VARS 0b00000001
#define LAYER_FLAG_HAS_REGISTRY 0b00000010
#define LAYER_FLAG_STATIC 0b00000100

/* Previous var pool stored raw blob pointers which could be handed to scripts.
    We now store blob pointers behind a handle table so external code (Lua) can
    keep a small, stable handle object and validate it before use. */

/* A var_handle is represented as a 32-bit handle32 value. For storage inside
    block metadata (which may store up to `var_index_size` bytes) we pack the
    handle32 -> u32 -> u64 when copying into block data. */
typedef handle32 var_handle;

/* Each layer that supports vars keeps a pointer to a handle table that owns
    blob pointers. The table is created at layer init and destroyed at free.
    The table's `type` tag should be set to a dedicated value (e.g. 1). */
typedef struct var_handle_table
{
     handle_table *table; /* stores blob* */
     u16 type_tag;        /* 6-bit type tag used when inserting into table */
} var_handle_table;

typedef struct layer
{
    u64 uuid;
    void *parent_room;

    var_handle_table var_pool;
    block_registry *registry;
    u8 *blocks; // array of blocks, each of size bytes_per_block

    u32 width;                //
    u32 height;               //
    u8 block_size;            // bytes per block id
    u8 var_index_size;        // bytes per var index
    // since we now use 32 bit handles it will be 4 bytes
    u8 total_bytes_per_block; // sum of block_size and var_index_size

    u8 flags; //
} layer;

typedef struct level level;

typedef struct room
{
    u64 uuid;
    char *name;
    level *parent_level;

    u32 width;
    u32 height;

    vec_void_t layers;
    // physics_world_t physics_world; /* each room gets its own physics world */

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

u8 block_get_vars(layer *l, u32 x, u32 y, blob **vars_out);

u32 block_get_vars_index(layer *l, u32 x, u32 y);
void block_var_index_set(layer *l, u32 x, u32 y, u32 index);

u8 block_delete_vars(layer *l, u32 x, u32 y);
u8 block_copy_vars(layer *l, u32 x, u32 y, blob vars);

// u8 layer_clean_vars(layer *l);

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
// turns an ascii string into formatted block chain
// supposed to be used with a special character block

void bprintf(layer *l, const u64 character_block_id, u32 orig_x, u32 orig_y, u32 length_limit, const char *format, ...);

#endif