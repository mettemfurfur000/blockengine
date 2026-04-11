#ifndef LEVEL_H
#define LEVEL_H 1

#include "block_registry.h"

#include "general.h"
#include "handle.h"
#include "hashtable.h"
#include "spatial_grid.h"

#include "block_entity.h"
#include "update_system.h"

#include "vec/src/vec.h"

#define LAYER_FLAG_USE_VARS (1 << 0)
#define LAYER_FLAG_HAS_REGISTRY (1 << 1)
#define LAYER_FLAG_STATIC (1 << 2)
#define LAYER_FLAG_HAS_ENTITIES (1 << 3)

/* Each layer that supports vars keeps a pointer to a handle table that owns
	blob pointers. The table is created at layer init and destroyed at free.
	The table's `type` tag should be set to a dedicated value (e.g. 1). */
typedef struct
{
	handle_table *table;
} var_handle_table;

typedef struct layer
{
	u64 uuid;
	void *parent_room;

	var_handle_table var_pool;
	block_registry *registry;
	u8 *blocks;

	u16 width;
	u16 height;
	u8 block_size;
	u8 total_bytes_per_block;

	u8 flags;

	spatial_grid spatial;

	handle_table *block_entity_pool;
	u32 block_entity_count_estimate;

	update_accumulator id_updates;
	update_accumulator var_updates;
	update_accumulator var_component_updates;
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
u8 block_move(layer *l, u16 x, u16 y, i16 dx, i16 dy);

u8 block_apply_updates(layer *l);

u8 block_get_vars(const layer *l, u16 x, u16 y, blob **vars_out);

void block_set_var_handle(layer *l, u16 x, u16 y, handle32 handle);
handle32 block_get_var_handle(layer *l, u16 x, u16 y);

handle32 var_table_alloc_blob(var_handle_table *pool, blob vars, bool parsed);
void var_table_free_handle(var_handle_table *pool, handle32 h);
handle32 layer_copy_new_vars(layer *l, blob vars);

u8 block_delete_vars(layer *l, u16 x, u16 y);
u8 layer_copy_vars(layer *l, u16 x, u16 y, blob vars);

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

void layer_build_spatial_grid(layer *l);

handle32 layer_add_block_entity(layer *l, u64 block_id, float x, float y);
void layer_remove_block_entity(layer *l, handle32 h);
block_entity *layer_get_block_entity(layer *l, handle32 h);
void layer_tick_entities(layer *l, float dt);

bool layer_block_entity_is_valid(layer *l, handle32 h);

#endif