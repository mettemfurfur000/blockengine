#include "include/block_entity.h"

#include "include/block_registry.h"
#include "include/handle.h"
#include "include/level.h"
#include "include/logging.h"
#include "include/sdl2_basics.h"
#include "include/uuid.h"

#include "include/vec_math.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

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

	// box2d create body

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = (b2Vec2){x, y};
	b2BodyId bodyId = b2CreateBody(((room *)l->parent_room)->b2_world_id, &bodyDef);

	b2Polygon dynamicBox = b2MakeBox(g_block_width * 0.5f, g_block_width * 0.5f);

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.0f;
	shapeDef.material.friction = 0.3f;

	e->b2_body_id = bodyId;

	b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);

	e->pos_old.x = x;
	e->pos_old.y = y;
	e->timestamp_old = 0;
	e->scale_x = 1.0f;
	e->scale_y = 1.0f;

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