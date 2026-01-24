#ifndef LEVEL_H
#define LEVEL_H 1

#include "vec/src/vec.h"
#include "block_registry.h"
#include "general.h"
#include "handle.h"
#include "hashtable.h"

#include "update_system.h"

#define LAYER_FLAG_USE_VARS 0b00000001
#define LAYER_FLAG_HAS_REGISTRY 0b00000010
#define LAYER_FLAG_STATIC 0b00000100

/* Each layer that supports vars keeps a pointer to a handle table that owns
    blob pointers. The table is created at layer init and destroyed at free.
    The table's `type` tag should be set to a dedicated value (e.g. 1). */
typedef struct
{
    handle_table *table; /* stores blob* */
} var_handle_table;

typedef struct layer
{
    u64 uuid;
    void *parent_room;

    var_handle_table var_pool;
    block_registry *registry;
    u8 *blocks; // array of blocks, each of size bytes_per_block

    u16 width;         //
    u16 height;        //
    u8 block_size;     // bytes per block id
    u8 total_bytes_per_block; // block_size + 4 (handle32)

    u8 flags; //
    
    update_acc updates; // accumulator for block updates on this layer
} layer;

typedef struct level level;

typedef struct room
{
    u64 uuid;
    char *name;
    level *parent_level;

    u16 width;
    u16 height;

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

#define BLOCK_ID_PTR(l, x, y) (l->blocks + (((y) * l->width) + (x)) * l->total_bytes_per_block)
#define MERGE32_TO_64(a, b) (((u64)a << 32) | (u64)b)

u8 block_set_id(layer *l, u16 x, u16 y, u64 id);
u8 block_get_id(layer *l, u16 x, u16 y, u64 *id);
u8 block_move(layer *l, u16 x, u16 y, u32 dx, u32 dy);

u8 block_get_vars(const layer *l, u16 x, u16 y, blob **vars_out);

u32 block_get_vars_index(layer *l, u16 x, u16 y);
void block_var_index_set(layer *l, u16 x, u16 y, u32 index);

u8 block_delete_vars(layer *l, u16 x, u16 y);
u8 block_copy_vars(layer *l, u16 x, u16 y, blob vars);

// u8 layer_clean_vars(layer *l);

// call after setting their resolutions, not recommended to use
u8 init_layer(layer *l, room *parent_room);
u8 init_room(room *r, level *parent_level);
u8 init_level(level *l);

u8 free_layer(layer *l);
u8 free_room(room *r);
u8 free_level(level *l);

// more usable functions to create levels, rooms, layers

level *level_create(const char *name);
room *room_create(level *parent, const char *name, u32 w, u32 h);
layer *layer_create(room *parent, block_registry *registry_ref, u8 bytes_per_block, u8 flags);

// utils
// turns an ascii string into formatted block chain
// supposed to be used with a special character block

void bprintf(layer *l, const u64 character_block_id, u32 orig_x, u32 orig_y, u32 length_limit, const char *format, ...);

#endif