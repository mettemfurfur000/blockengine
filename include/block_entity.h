#ifndef BLOCK_ENTITY_H
#define BLOCK_ENTITY_H 1

#include "general.h"
#include "handle.h"
#include "hashtable.h"

struct layer;

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct block_entity block_entity;

	struct block_entity
	{
		struct layer *parent_layer;
		handle32 handle;	 // handle in the layer's block_entity_pool
		handle32 var_handle; // handle to block vars in the layer's var_pool (optional)

		u64 uuid;
		u64 block_id;

		f32 x, y;
		f32 vx, vy;
		f32 rotation;
		f32 scale_x, scale_y;
		f32 old_x, old_y;
		u32 timestamp_old;

		u8 flags;
	};

	block_entity *block_entity_create(struct layer *parent, u64 block_id, float x, float y);
	void block_entity_destroy(block_entity *e);
	block_entity *block_entity_get(struct layer *parent, handle32 h);
	bool block_entity_is_valid(handle_table *table, handle32 h);

	u8 block_entity_get_vars(const block_entity *e, blob **vars_out);
	void block_entity_set_var_handle(block_entity *e, handle32 handle);

	void block_entity_update(block_entity *e, float dt);

	void block_entity_apply_physics(block_entity *e, struct layer *collider_layer, float dt);
	bool block_entity_check_collision(block_entity *e, struct layer *collider_layer);

#ifdef __cplusplus
}
#endif

#endif