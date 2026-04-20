#include "include/block_entity.h"

#include "include/block_registry.h"
#include "include/handle.h"
#include "include/level.h"
#include "include/logging.h"
#include "include/uuid.h"

#include "include/vec_math.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static collision_result check_collision_at(layer *collider_layer, vec2 pos)
{
	collision_result result = {
		.block_pos = {-1, -1},
	};

	i32 tile_x = (i32)floor(pos.x / g_block_width);
	i32 tile_y = (i32)floor(pos.y / g_block_width);

	if (tile_x < 0 || tile_x >= (i32)collider_layer->width || tile_y < 0 || tile_y >= (i32)collider_layer->height)
		return result;

	block_get_id(collider_layer, (u16)tile_x, (u16)tile_y, &result.block_id);

	if (result.block_id != 0)
	{
		result.block_pos.x = tile_x;
		result.block_pos.y = tile_y;
	}

	return result;
}

static vec2 rotate_point(vec2 point, vec2 center, f32 angle_rad)
{
	f32 cos_a = cosf(angle_rad);
	f32 sin_a = sinf(angle_rad);

	f32 dx = point.x - center.x;
	f32 dy = point.y - center.y;

	return (vec2){
		.x = center.x + dx * cos_a - dy * sin_a,
		.y = center.y + dx * sin_a + dy * cos_a,
	};
}

bool block_entity_check_collision(block_entity *e, layer *collider_layer)
{
	assert(e);
	assert(collider_layer);

	vec2 pos = e->pos;
	f32 half_w = (g_block_width * e->scale_x) * 0.5f;
	f32 half_h = (g_block_width * e->scale_y) * 0.5f;

	vec2 corners[4] = {
		{.x = pos.x - half_w, .y = pos.y - half_h},
		{.x = pos.x + half_w, .y = pos.y - half_h},
		{.x = pos.x + half_w, .y = pos.y + half_h},
		{.x = pos.x - half_w, .y = pos.y + half_h},
	};

	for (i32 i = 0; i < 4; i++)
	{
		vec2 rotated = rotate_point(corners[i], pos, DEG_TO_RAD(e->rotation));
		collision_result res = check_collision_at(collider_layer, rotated);
		if (res.block_id != 0)
			return true;
	}

	return false;
}

bool block_entity_get_collision_info(block_entity *e, layer *collider_layer, collision_result *ret)
{
	assert(e);
	assert(collider_layer);

	vec2 pos = e->pos;
	f32 half_w = (g_block_width * e->scale_x) * 0.5f;
	f32 half_h = (g_block_width * e->scale_y) * 0.5f;

	vec2 corners[4] = {
		{.x = pos.x - half_w, .y = pos.y - half_h},
		{.x = pos.x + half_w, .y = pos.y - half_h},
		{.x = pos.x + half_w, .y = pos.y + half_h},
		{.x = pos.x - half_w, .y = pos.y + half_h},
	};

	for (i32 i = 0; i < 4; i++)
	{
		vec2 rotated = rotate_point(corners[i], pos, DEG_TO_RAD(e->rotation));
		collision_result res = check_collision_at(collider_layer, rotated);
		if (res.block_id != 0)
		{
			*ret = res;
			return true;
		}
	}

	return false;
}

bool block_entity_check_collision_swept(block_entity *e, layer *collider_layer, f32 dt, collision_result *ret)
{
	assert(e);
	assert(collider_layer);

	f32 half_w = (g_block_width * e->scale_x) * 0.5f;
	f32 half_h = (g_block_width * e->scale_y) * 0.5f;

	vec2 start_pos = e->pos_old;
	vec2 end_pos = e->pos;

	f32 dx = end_pos.x - start_pos.x;
	f32 dy = end_pos.y - start_pos.y;
	f32 dist = sqrtf(dx * dx + dy * dy);

	if (dist < 0.001f)
	{
		return block_entity_get_collision_info(e, collider_layer, ret);
	}

	f32 step_size = g_block_width * 0.25f;
	i32 steps = (i32)ceilf(dist / step_size);

	if (steps < 1)
		steps = 1;
	if (steps > 100)
		steps = 100;

	f32 inv_steps = 1.0f / (f32)steps;

	for (i32 s = 1; s <= steps; s++)
	{
		f32 t = (f32)s * inv_steps;
		vec2 pos = {
			.x = start_pos.x + dx * t,
			.y = start_pos.y + dy * t,
		};

		vec2 corners[4] = {
			{.x = pos.x - half_w, .y = pos.y - half_h},
			{.x = pos.x + half_w, .y = pos.y - half_h},
			{.x = pos.x + half_w, .y = pos.y + half_h},
			{.x = pos.x - half_w, .y = pos.y + half_h},
		};

		for (i32 i = 0; i < 4; i++)
		{
			vec2 rotated = rotate_point(corners[i], pos, DEG_TO_RAD(e->rotation));
			collision_result res = check_collision_at(collider_layer, rotated);
			if (res.block_id != 0)
			{
				*ret = res;
				ret->hit_pos.x = rotated.x;
				ret->hit_pos.y = rotated.y;
				return true;
			}
		}
	}

	return false;
}

handle32 layer_add_block_entity(layer *l, u64 block_id, float x, float y)
{
	assert(l);

	if (!l->block_entity_pool)
	{
		LOG_ERROR("Attempted to add block entity to layer %lld which does not have a block_entity_pool", l->uuid);
		return INVALID_HANDLE;
	}

	block_entity *e = (block_entity *)malloc(sizeof(block_entity));
	assert(e);

	memset(e, 0, sizeof(block_entity));

	e->uuid = generate_uuid();
	e->parent_layer = l;
	e->block_id = block_id;

	do
	{
		if (!l->var_pool.table)
			break;

		block_registry *reg = l->registry;
		block_resources *res = reg->resources.data + block_id;
		if (res->vars_sample.size == 0 || res->vars_sample.ptr == NULL)
			break;

		handle32 newh = layer_copy_new_vars(l, res->vars_sample);
		if (newh.index == INVALID_HANDLE_INDEX)
			LOG_ERROR("Failed to copy vars for new block entity on layer %lld", l->uuid);
		else
			e->var_handle = newh;

	} while (0);

	e->pos.x = x;
	e->pos.y = y;
	e->pos_old.x = x;
	e->pos_old.y = y;
	e->timestamp_old = 0;
	e->scale_x = 1.0f;
	e->scale_y = 1.0f;

	e->mass = 1;

	handle32 h = handle_table_put(l->block_entity_pool, e);
	if (!handle_is_valid(l->block_entity_pool, h))
	{
		LOG_ERROR("Failed to add block entity to layer %lld", l->uuid);
		free(e);
		return INVALID_HANDLE;
	}

	e->handle = h;
	l->flags |= LAYER_FLAG_HAS_ENTITIES;

	return h;
}

void block_entity_destroy(block_entity *e)
{
	assert(e);

	layer *l = e->parent_layer;
	if (l && l->block_entity_pool)
		handle_table_release(l->block_entity_pool, e->handle);

	free(e);
}

block_entity *block_entity_get(layer *parent, handle32 h)
{
	assert(parent);
	assert(parent->block_entity_pool);

	if (!handle_is_valid(parent->block_entity_pool, h))
		return NULL;

	return (block_entity *)handle_table_get(parent->block_entity_pool, h);
}

bool block_entity_is_valid(handle_table *table, handle32 h)
{
	assert(table);
	return handle_is_valid(table, h);
}

void layer_remove_block_entity(layer *l, handle32 h)
{
	assert(l);
	assert(l->block_entity_pool);

	block_entity *e = block_entity_get(l, h);
	assert(e);
	block_entity_destroy(e);
}

u8 block_entity_get_vars(const block_entity *e, blob **vars_out)
{
	assert(e);
	assert(vars_out);
	assert(e->parent_layer);

	void *ptr = handle_table_get(e->parent_layer->var_pool.table, e->var_handle);
	if (!ptr)
	{
		*vars_out = NULL;
		return FAIL;
	}

	*vars_out = (blob *)ptr;
	return SUCCESS;
}

void block_entity_set_var_handle(block_entity *e, handle32 handle)
{
	assert(e);
	assert(e->parent_layer);

	if (!handle_is_valid(e->parent_layer->var_pool.table, handle))
	{
		LOG_ERROR("Attempted to set invalid var handle on block entity %lld, layer %lld", e->uuid,
				  e->parent_layer->uuid);
		return;
	}

	if (e->var_handle.index != INVALID_HANDLE_INDEX)
		var_table_free_handle(&e->parent_layer->var_pool, e->var_handle);

	e->var_handle = handle;
}

vec2 gravity = {0, 9.81f};

void block_entity_physics_step(block_entity *e, float dt)
{
	assert(e);

	e->timestamp_old = SDL_GetTicks();

	e->pos_old.x = e->pos.x; // save old pos
	e->pos_old.y = e->pos.y;

	e->force.x += gravity.x * e->mass; // add gravity
	e->force.y += gravity.y * e->mass;

	e->velocity.x += (e->force.x / e->mass) * dt; // apply forces
	e->velocity.y += (e->force.y / e->mass) * dt;

	e->pos.x += e->velocity.x * dt; // apply velocity
	e->pos.y += e->velocity.y * dt;

	e->force.x = 0; // remove forces
	e->force.y = 0;
}