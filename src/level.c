#include "include/level.h"

#include "include/logging.h"
#include "include/sdl2_basics.h"
#include "include/update_system.h"
#include "include/uuid.h"
#include "include/vars.h"

#include "include/flags.h"
#include "vec/src/vec.h"

#include <box2d/box2d.h>
#include <box2d/math_functions.h>
#include <lua.h>
#include <stdarg.h>
#include <stdio.h>

u8 block_get_id(layer *l, u16 x, u16 y, u64 *id)
{
	assert(l);
	assert(id);

	if (x >= l->width || y >= l->height)
		return FAIL;

	void *ptr = BLOCK_ID_PTR(l, x, y);

	switch (l->block_size)
	{
	case 1:
		*id = *(u8 *)ptr;
		return SUCCESS;
	case 2:
		*id = *(u16 *)ptr;
		return SUCCESS;
	case 4:
		*id = *(u32 *)ptr;
		return SUCCESS;
	case 8:
		*id = *(u64 *)ptr;
		return SUCCESS;
	}

	return FAIL;
}

u8 block_move(layer *l, u16 x, u16 y, i16 dx, i16 dy)
{
	assert(l);

	if (dx == 0 && dy == 0)
		return SUCCESS;

	u32 dest_x = x + dx;
	u32 dest_y = y + dy;

	if (x >= l->width || y >= l->height)
		return FAIL;
	if (dest_x >= l->width || dest_y >= l->height)
		return FAIL;

	// moves id
	u64 src_id = 0;
	block_get_id(l, x, y, &src_id);

	block_set_id(l, dest_x, dest_y, src_id);
	block_set_id(l, x, y, 0);

	if (l->var_pool.table == NULL) // skip the move if no var handles
		return SUCCESS;

	// moves handle
	handle32 h_src = block_get_var_handle(l, x, y);

	block_set_var_handle(l, dest_x, dest_y, h_src);
	block_set_var_handle(l, x, y, (handle32){});

	return SUCCESS;
}

u8 block_set_id(layer *l, u16 x, u16 y, u64 id)
{
	assert(l != NULL);
	if (x >= l->width || y >= l->height)
		return FAIL;

	u64 old_id = 0;
	void *ptr = BLOCK_ID_PTR(l, x, y);

	switch (l->block_size)
	{
	case 1:
		old_id = *(u8 *)ptr;
		*(u8 *)ptr = id;
		break;
	case 2:
		old_id = *(u16 *)ptr;
		*(u16 *)ptr = id;
		break;
	case 4:
		old_id = *(u32 *)ptr;
		*(u32 *)ptr = id;
		break;
	case 8:
		old_id = *(u64 *)ptr;
		*(u64 *)ptr = id;
		break;
	default:
		assert(0 || "unreachable");
		return FAIL;
	}

	// TODO: handle the block update and send

	spatial_grid_update(&l->spatial, x, y, old_id, id);
	return SUCCESS;
}

//
//
//

/* Create a new blob in the pool and return the handle. The blob memory is
   allocated and the contents copied from `vars`. Returns INVALID_HANDLE on
   failure.

   Set parsed to true if blob originates from a parse function
   */
handle32 var_table_alloc_blob(var_handle_table *pool, blob vars, bool parsed)
{
	if (!pool || !pool->table)
		return INVALID_HANDLE;

	blob *b = calloc(1, sizeof(blob));

	if (!b)
		return INVALID_HANDLE;

	if (!parsed)
	{
		b->ptr = calloc(vars.size ? vars.size : 1, 1);
		if (!b->ptr)
		{
			SAFE_FREE(b);
			return INVALID_HANDLE;
		}
		if (vars.size && vars.ptr)
			memcpy(b->ptr, vars.ptr, vars.size);
		b->size = vars.size;
	}
	else
	{
		*b = vars;
	}

	handle32 h = handle_table_put(pool->table, b);

	if (h.index == INVALID_HANDLE_INDEX)
	{
		SAFE_FREE(b->ptr);
		SAFE_FREE(b);
		return INVALID_HANDLE;
	}

	return h;
}

/* Free a blob previously stored in the table (frees memory and releases handle). */
void var_table_free_handle(var_handle_table *pool, handle32 h)
{
	if (!pool || !pool->table)
		return;
	blob *b = (blob *)handle_table_get(pool->table, h);
	if (b)
	{
		SAFE_FREE(b->ptr);
		SAFE_FREE(b);
	}
	handle_table_release(pool->table, h);
}

handle32 block_get_var_handle(layer *l, u16 x, u16 y)
{
	assert(l);

	if (x >= l->width || y >= l->height)
		return INVALID_HANDLE;
	if (l->var_pool.table == NULL)
		return INVALID_HANDLE;

	handle32 handle = {};
	memcpy((u8 *)&handle, BLOCK_ID_PTR(l, x, y) + l->block_size, sizeof(handle32));

	return handle;
}

void block_set_var_handle(layer *l, u16 x, u16 y, handle32 handle)
{
	assert(l);
	assert(l->blocks);
	if (x >= l->width || y >= l->height)
		return;
	if (l->var_pool.table == NULL)
		return;

	memcpy(BLOCK_ID_PTR(l, x, y) + l->block_size, (u8 *)&handle, sizeof(handle32));
}

u8 block_get_vars(const layer *l, u16 x, u16 y, blob **vars_out)
{
	assert(l);
	assert(vars_out);
	assert(l->blocks);
	if (x >= l->width || y >= l->height)
		return FAIL;
	if (l->var_pool.table == NULL)
		return FAIL;

	handle32 handle = {};
	memcpy((u8 *)&handle, BLOCK_ID_PTR(l, x, y) + l->block_size, sizeof(handle32));
	blob *b = (blob *)handle_table_get(l->var_pool.table, handle);

	if (!b)
	{
		*vars_out = NULL;
		return FAIL;
	}

	*vars_out = b;

	return SUCCESS;
}

u8 block_delete_vars(layer *l, u16 x, u16 y)
{
	assert(l);
	if (x >= l->width || y >= l->height)
		return FAIL;

	if (l->var_pool.table == NULL)
		return SUCCESS; // nothing to delete

	// block_push_var_change(l, x, y, INVALID_HANDLE);

	handle32 handle = INVALID_HANDLE;
	memcpy((u8 *)&handle, BLOCK_ID_PTR(l, x, y) + l->block_size, sizeof(handle32));

	var_table_free_handle(&l->var_pool, handle);

	handle = INVALID_HANDLE;
	memcpy(BLOCK_ID_PTR(l, x, y) + l->block_size, (u8 *)&handle, sizeof(handle32));

	return SUCCESS;
}

handle32 layer_copy_new_vars(layer *l, blob vars)
{
	assert(l);

	if (l->var_pool.table == NULL)
		return INVALID_HANDLE;

	handle32 newh = var_table_alloc_blob(&l->var_pool, vars, false);
	if (newh.index == INVALID_HANDLE_INDEX)
	{
		LOG_ERROR("Failed to create a new var");
		return INVALID_HANDLE;
	}

	blob *blob_ptr = handle_table_get(l->var_pool.table, newh);
	assert(blob_ptr);
	memcpy(blob_ptr->ptr, vars.ptr, vars.size);

	return newh;
}

u8 layer_copy_vars(layer *l, u16 x, u16 y, blob vars)
{
	assert(l);
	if (x >= l->width || y >= l->height)
		return FAIL;

	if (l->var_pool.table == NULL)
		return SUCCESS;

	if (vars.size == 0 || vars.ptr == NULL)
	{
		if (block_delete_vars(l, x, y) != SUCCESS)
		{
			LOG_ERROR("Failed to delete vars for %d:%d, layer %lld", x, y, l->uuid);
			return FAIL;
		}

		return SUCCESS;
	}

	handle32 existing = block_get_var_handle(l, x, y);
	blob *blob_ptr = NULL;

	// try getting an existing var and reusing it for new vars
	if (handle_is_valid(l->var_pool.table, existing))
	{
		blob_ptr = (blob *)handle_table_get(l->var_pool.table, existing);
		if (blob_ptr && blob_ptr->size >= vars.size)
		{
			blob_ptr->size = vars.size;
			memcpy(blob_ptr->ptr, vars.ptr, vars.size);

			return SUCCESS;
		}

		/* free existing and allocate new below */
		block_delete_vars(l, x, y);
	}

	// have to create a new var handle for this

	handle32 newh = layer_copy_new_vars(l, vars);
	if (newh.index == INVALID_HANDLE_INDEX)
	{
		LOG_ERROR("Failed to copy vars for %d:%d, layer %lld", x, y, l->uuid);
		return FAIL;
	}

	block_set_var_handle(l, x, y, newh);

	return SUCCESS;
}

// init functions

u8 init_layer(layer *l, room *parent_room)
{
	assert(parent_room);
	assert(l);

	l->parent_room = parent_room;
	l->uuid = generate_uuid();

	if (l->width == 0 || l->height == 0)
		return FAIL;
	if ((l->block_size + sizeof(handle32)) == 0)
		return FAIL;
	l->total_bytes_per_block = l->block_size + sizeof(handle32);

	l->blocks = calloc(l->width * l->height, l->block_size + sizeof(handle32));

	if (l->blocks == NULL)
	{
		LOG_ERROR("Failed to allocate memory for blocks");
		return FAIL;
	}

	l->spatial = spatial_grid_create(l->width, l->height, 16);

	if (FLAG_GET(l->flags, LAYER_FLAG_USE_VARS))
	{
		l->var_pool.table = handle_table_create(256);
	}

	if (FLAG_GET(l->flags, LAYER_FLAG_HAS_ENTITIES))
	{
		l->block_entity_pool = handle_table_create(256);
	}

	return SUCCESS;
}

void room_create_world(room *r_target, room *shared)
{
	if (shared)
	{
		r_target->b2_world_id = shared->b2_world_id;
		return;
	}

	b2WorldDef worldDef = b2DefaultWorldDef();

	worldDef.gravity = (b2Vec2){0, 16 * 10};
	worldDef.enableSleep = false;

	r_target->b2_world_id = b2CreateWorld(&worldDef);
}

u8 init_room(room *r, level *parent_level)
{
	assert(parent_level);
	assert(r);

	r->parent_level = parent_level;
	r->uuid = generate_uuid();

	vec_init(&r->layers);

	// TODO: allow rooms to share the same physics world, probably move this to a physics creation function
	// and allow passing another room as a reference to get the world handle from

	room_create_world(r, NULL);

	assert(r->name);

	return SUCCESS;
}

u8 init_level(level *lvl)
{
	assert(lvl);

	vec_init(&lvl->rooms);
	vec_init(&lvl->registries);

	lvl->uuid = generate_uuid();

	assert(lvl->name);

	return SUCCESS;
}

// free functions

u8 free_layer(layer *l)
{
	assert(l);
	assert(l->blocks);

	spatial_grid_destroy(&l->spatial);

	if (l->var_pool.table)
	{
		u16 cap = handle_table_capacity(l->var_pool.table);
		for (u16 i = 0; i < cap; ++i)
		{
			void *p = handle_table_slot_ptr(l->var_pool.table, i);
			if (p)
			{
				blob *b = (blob *)p;
				SAFE_FREE(b->ptr);
				SAFE_FREE(b);
			}
		}
		handle_table_destroy(l->var_pool.table);
		l->var_pool.table = NULL;
	}

	if (l->block_entity_pool)
	{
		handle_table_destroy(l->block_entity_pool);
		l->block_entity_pool = NULL;
	}

	SAFE_FREE(l->blocks);

	l->uuid = 0;

	return SUCCESS;
}

u8 free_room(room *r)
{
	assert(r);

	for (u32 i = 0; i < r->layers.length; i++)
		free_layer(r->layers.data[i]);

	vec_deinit(&r->layers);
	SAFE_FREE(r->name);

	// if (r->physics_world)
	// {
	//     physics_world_destroy(r->physics_world);
	//     r->physics_world = NULL;
	// }

	r->uuid = 0;

	return SUCCESS;
}

u8 free_level(level *lvl)
{
	assert(lvl);

	LOG_DEBUG("Freeing level %s, the world might collapse!", lvl->name);

	for (u32 i = 0; i < lvl->rooms.length; i++)
		free_room(lvl->rooms.data[i]);

	vec_deinit(&lvl->rooms);
	vec_deinit(&lvl->registries);

	lvl->uuid = 0;

	SAFE_FREE(lvl->name);
	return SUCCESS;
}

// more useful functions

level *level_create(const char *name)
{
	level *lvl = calloc(1, sizeof(level));

	if (!lvl)
		return NULL;

	lvl->name = strdup(name);
	init_level(lvl);

	return lvl;
}

room *room_create(level *parent, const char *name, u32 w, u32 h)
{
	room *r = calloc(1, sizeof(room));

	if (!r)
		return NULL;

	r->name = strdup(name);
	r->width = w;
	r->height = h;
	r->parent_level = parent;

	init_room(r, parent);

	(void)vec_push(&parent->rooms, r);

	return r;
}

layer *layer_create(room *parent, block_registry *registry_ref, u8 bytes_per_block, u8 flags)
{
	layer *l = calloc(1, sizeof(layer));

	if (!l)
		return NULL;

	l->registry = registry_ref;
	l->block_size = bytes_per_block;

	assert(bytes_per_block <= 8 && bytes_per_block > 0);
	assert(bytes_per_block == 1 || bytes_per_block % 2 == 0);

	l->total_bytes_per_block = bytes_per_block + sizeof(handle32);
	l->width = parent->width;
	l->height = parent->height;
	l->flags = flags;

	init_layer(l, parent);

	(void)vec_push(&parent->layers, l);

	return l;
}

void bprintf(layer *l, const u64 letter_block_id, u32 orig_x, u32 orig_y, u32 length_limit, const char *format, ...)
{
	assert(l);
	assert(l->blocks);

	char buffer[1024] = {0};
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);

	char *ptr = buffer;
	int x = orig_x;
	int y = orig_y;

	blob vars_scratch = {};

	blob_dup(&vars_scratch, l->registry->resources.data[letter_block_id].vars_sample);

	char c = ' ';

	while (*ptr != 0)
	{
		c = iscntrl(*ptr) ? ' ' : *ptr;
		if (c == ' ')
		{
			block_delete_vars(l, x, y);
			block_set_id(l, x, y, 0);
		}
		else
		{
			bool write_new = false;
			block_set_id(l, x, y, letter_block_id);

			blob *vars = NULL;
			if (block_get_vars(l, x, y, &vars) != SUCCESS)
			{
				write_new = true;
				vars = &vars_scratch;
			}

			if (var_set_u8(vars, 'v', c) != SUCCESS)
				LOG_WARNING("bprintf Failed to set a u8 var for char block at %d:%d", x, y);

			if (write_new)
				layer_copy_vars(l, x, y, vars_scratch);
		}

		switch (*ptr)
		{
		case '\n':
			x = orig_x;
			y++;
			break;
		case '\t':
			x += 4;
			break;
		default:
			x++;
		}

		if (x >= length_limit || x >= l->width)
		{
			x = orig_x;
			y++;
		}

		ptr++;
	}

	SAFE_FREE(vars_scratch.ptr);
	va_end(args);
}

void layer_build_spatial_grid(layer *l)
{
	if (!l || !l->blocks)
		return;
	spatial_grid_build_from_layer(&l->spatial, l->width, l->height, l->block_size, l->blocks, l->total_bytes_per_block);
}

bool layer_block_entity_is_valid(layer *l, handle32 h)
{
	if (!l || !l->block_entity_pool)
		return false;
	return handle_is_valid(l->block_entity_pool, h);
}

block_entity *layer_get_block_entity(layer *l, handle32 h)
{
	if (!l || !l->block_entity_pool)
		return NULL;
	return block_entity_get(l, h);
}

static u32 cleanup_iter_entity(handle32 h, void *ptr, void *user_data)
{
	struct
	{
		u16 *used_handles;
		u16 capacity;
	} *ctx = user_data;

	block_entity *e = (block_entity *)ptr;
	if (e->var_handle.index != INVALID_HANDLE_INDEX && e->var_handle.index < ctx->capacity)
	{
		ctx->used_handles[e->var_handle.index] = 1;
	}
	return SUCCESS;
}

u8 layer_cleanup_unused_vars(layer *l)
{
	if (!l || !l->var_pool.table)
		return FAIL;

	u16 capacity = handle_table_capacity(l->var_pool.table);
	u16 *used_handles = calloc(capacity, sizeof(u16));
	if (!used_handles)
		return FAIL;

	for (u16 y = 0; y < l->height; y++)
	{
		for (u16 x = 0; x < l->width; x++)
		{
			handle32 h = block_get_var_handle(l, x, y);
			if (h.index != INVALID_HANDLE_INDEX && h.index < capacity)
			{
				used_handles[h.index] = 1;
			}
		}
	}

	if (l->block_entity_pool)
	{
		struct
		{
			u16 *used_handles;
			u16 capacity;
		} ctx = {.used_handles = used_handles, .capacity = capacity};

		handle_table_iterate(l->block_entity_pool, cleanup_iter_entity, &ctx);
	}

	LOG_INFO("Checking var pool for unused handles (capacity: %u)", capacity);
	u32 cleaned = 0;
	for (u16 i = 1; i < capacity; i++)
	{
		if (!used_handles[i] && handle_table_slot_active(l->var_pool.table, i))
		{
			handle32 h = {.index = i, .active = 1};
			blob *b = (blob *)handle_table_get(l->var_pool.table, h);
			if (b && b->ptr)
			{
				char buffer[256] = {0};
				dbg_data_layout(*b, buffer);
				LOG_INFO("Cleaning unused var handle at index %u: %s", i, buffer);
				var_table_free_handle(&l->var_pool, h);
				cleaned++;
			}
		}
	}

	LOG_INFO("Cleaned up %u unused var handles", cleaned);
	free(used_handles);
	return SUCCESS;
}

// void layer_build_ground_physics(layer *l)
// {
// 	assert(l);
// 	b2WorldId world = ((room *)l->parent_room)->b2_world_id;
// 	assert(b2World_IsValid(world));

// 	f32 block_world_size = (f32)g_block_width;

// 	bool *used = alloca(l->width * l->height);
// 	assert(used);

// 	for (u32 y = 0; y < l->height; y++)
// 	{
// 		for (u32 x = 0; x < l->width; x++)
// 		{
// 			u32 idx = y * l->width + x;
// 			if (used[idx])
// 				continue;

// 			u64 id = 0;
// 			block_get_id(l, (u16)x, (u16)y, &id);
// 			if (id == 0)
// 			{
// 				used[idx] = true;
// 				continue;
// 			}

// 			u16 width = 1;
// 			while (x + width < l->width)
// 			{
// 				u32 next_idx = y * l->width + (x + width);
// 				if (used[next_idx])
// 					break;
// 				u64 next_id = 0;
// 				block_get_id(l, (u16)(x + width), (u16)y, &next_id);
// 				if (next_id == 0)
// 					break;
// 				width++;
// 			}

// 			u16 height = 1;
// 			bool can_extend_down = true;
// 			while (can_extend_down && y + height < l->height)
// 			{
// 				for (u16 w = 0; w < width; w++)
// 				{
// 					u32 check_idx = (y + height) * l->width + (x + w);
// 					if (used[check_idx])
// 					{
// 						can_extend_down = false;
// 						break;
// 					}
// 					u64 check_id = 0;
// 					block_get_id(l, (u16)(x + w), (u16)(y + height), &check_id);
// 					if (check_id == 0)
// 					{
// 						can_extend_down = false;
// 						break;
// 					}
// 				}
// 				if (can_extend_down)
// 					height++;
// 			}

// 			for (u16 dy = 0; dy < height; dy++)
// 			{
// 				for (u16 dx = 0; dx < width; dx++)
// 				{
// 					u32 mark_idx = (y + dy) * l->width + (x + dx);
// 					used[mark_idx] = true;
// 				}
// 			}

// 			f32 box_width = (f32)width * block_world_size;
// 			f32 box_height = (f32)height * block_world_size;
// 			// TODO: when multiple rooms share a box2d world, use relative room coordinates when calculatink static body
// 			// coordinates

// 			// f32 pos_x = (f32)x * block_world_size;
// 			// f32 pos_y = (f32)y * block_world_size;
// 			f32 pos_x = (f32)x * block_world_size + box_width * 0.5f;
// 			f32 pos_y = (f32)y * block_world_size + box_height * 0.5f;

// 			b2BodyDef bodyDef = b2DefaultBodyDef();
// 			bodyDef.type = b2_staticBody;
// 			bodyDef.position = (b2Vec2){pos_x, pos_y};

// 			b2BodyId body = b2CreateBody(world, &bodyDef);
// 			if (!b2Body_IsValid(body))
// 				continue;

// 			b2Polygon box = b2MakeBox(box_width * 0.5f, box_height * 0.5f);
// 			b2ShapeDef shapeDef = b2DefaultShapeDef();

// 			b2CreatePolygonShape(body, &shapeDef, &box);
// 		}
// 	}
// }

void layer_build_ground_physics(layer *l)
{
	assert(l);
	b2WorldId world = ((room *)l->parent_room)->b2_world_id;
	assert(b2World_IsValid(world));

	for (u32 y = 0; y < l->height; y++)
	{
		for (u32 x = 0; x < l->width; x++)
		{
			u64 id = 0;
			block_get_id(l, (u16)x, (u16)y, &id);
			if (id == 0)
				continue;

			f32 pos_x = (f32)x * g_block_width - g_block_width * 0.5f + g_block_width;
			f32 pos_y = (f32)y * g_block_width - g_block_width * 0.5f + g_block_width;

			b2BodyDef bodyDef = b2DefaultBodyDef();
			bodyDef.type = b2_staticBody;
			bodyDef.position = (b2Vec2){pos_x, pos_y};

			b2BodyId body = b2CreateBody(world, &bodyDef);
			if (!b2Body_IsValid(body))
				continue;

			b2Polygon box = b2MakeBox(g_block_width * 0.5f, g_block_width * 0.5f);
			b2ShapeDef shapeDef = b2DefaultShapeDef();

			b2CreatePolygonShape(body, &shapeDef, &box);
		}
	}
}