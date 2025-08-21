#ifndef ENTITY_H
#define ENTITY_H

#include "general.h"
#include "handle.h"
#include "level.h"

#include "physics.h"
#include <box2d/box2d.h>
#include <box2d/math_functions.h>

#include "../vec/src/vec.h"

typedef vec_t(handle32) handle32_vec;

typedef enum entity_type
{
    ENTITY_TYPE_INVALID = 0,
    ENTITY_TYPE_POINT = 1,
    ENTITY_TYPE_FULL = 2,
} entity_type;

typedef struct point_entity
{
    u64 uuid;
    char *name;
    /* point entities may have an associated physics body; physics state is stored in the body */
    physics_body_t body;
    void *user_data;
} point_entity;

typedef struct full_entity
{
    u64 uuid;
    char *name;

    room body;
    u64 collision_layer_mask;
    physics_body_t body_phys;
    physics_joint_t parent_joint;
    void *user_data;
    /* pointer to the room where this entity actually lives (used for physics/world)
        may be NULL if the entity is not placed in a room with a physics world */
    room *parent_room;
} full_entity;

typedef struct entity
{
    u64 uuid;
    entity_type type;
    char *name;

    handle32 parent;
    handle32_vec children;
    union
    {
        point_entity point;
        full_entity full;
    } payload;
} entity;

typedef struct entity_table
{
    handle_table *handles;
} entity_table;

entity_table *entity_table_create(u16 capacity);
void entity_table_destroy(entity_table *t);
handle32 entity_table_put(entity_table *t, entity *e, u16 type_tag);
entity *entity_table_get(entity_table *t, handle32 h);
void entity_table_release(entity_table *t, handle32 h);
entity *entity_table_find_by_uuid(entity_table *t, u64 uuid);

/* Convenience constructors (first arg may be NULL to use global table) */
handle32 entity_create_point(entity_table *t, const char *name, b2Vec2 pos);
/* layer_index selects which layer inside `body` is used to generate the physics shapes (0-based).
    Pass UINT_MAX to use a default (0).
    `parent_room` is the room where this entity will be placed (used to find the physics world).
    If `parent_room` is NULL, the body.room's `parent_level` may still be used by callers, but callers
    should pass an explicit parent room when available. */
handle32 entity_create_full(entity_table *t, const char *name, room body, room *parent_room, unsigned layer_index,
                            u64 collision_mask);

/* Global entity table (lazy-created by bindings if needed) */
extern entity_table *g_entity_table;

bool entity_attach(entity_table *t, handle32 parent, handle32 child);
bool entity_detach(entity_table *t, handle32 child);

static inline bool entity_full_layer_collides(const entity *e, unsigned layer_index)
{
    if (!e || e->type != ENTITY_TYPE_FULL)
        return false;
    if (layer_index >= 64)
        return false;
    return (e->payload.full.collision_layer_mask & (1ull << layer_index)) != 0;
}

#endif
