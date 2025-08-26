#include "../include/entity.h"
#include "../include/handle.h"
#include "../include/uuid.h"
#include <stdlib.h>

// Global entity table used by scripting bindings
entity_table *g_entity_table = NULL;

// Convenience wrappers for C callers that don't manage an entity_table
handle32 entity_create_point(entity_table *t, const char *name, b2Vec2 pos)
{
    if (!t)
    {
        if (!g_entity_table)
            g_entity_table = entity_table_create(1024);
        t = g_entity_table;
    }

    if (!t)
        return INVALID_HANDLE;

    entity *e = calloc(1, sizeof(*e));

    if (!e)
    {
        LOG_ERROR("allocation failed for a point entity");
        return INVALID_HANDLE;
    }

    e->uuid = generate_uuid();
    e->type = ENTITY_TYPE_POINT;
    e->name = name ? strdup(name) : NULL;
    /* point entities currently do not create a physics body by default; store pos in a transient body if needed later
     */
    e->payload.point.body = NULL;
    e->parent = INVALID_HANDLE;

    vec_init(&e->children);

    return entity_table_put(t, e, 0);
}

/* Helper: free a single entity's resources (does not free child entities).
   Leaves children handles untouched; caller should release child handles
   separately if desired. */
static void entity_free_shallow(entity *e)
{
    if (!e)
        return;
    free(e->name);
    if (e->type == ENTITY_TYPE_FULL)
    {
        /* free the room contents */
        free_room(&e->payload.full.body);
    }
    /* free children vector storage */
    vec_deinit(&e->children);
}

entity_table *entity_table_create(u16 capacity)
{
    entity_table *t = malloc(sizeof(*t));
    if (!t)
        return NULL;
    t->handles = handle_table_create(capacity);
    if (!t->handles)
    {
        free(t);
        return NULL;
    }
    return t;
}

void entity_table_destroy(entity_table *t)
{
    if (!t)
        return;
    u16 cap = handle_table_capacity(t->handles);
    for (u16 i = 0; i < cap; ++i)
    {
        if (!handle_table_slot_active(t->handles, i))
            continue;
        entity *e = (entity *)handle_table_slot_ptr(t->handles, i);
        if (e)
        {
            entity_free_shallow(e);
            free(e);
        }
    }
    handle_table_destroy(t->handles);
    free(t);
}

handle32 entity_table_put(entity_table *t, entity *e, u16 type_tag)
{
    if (!t || !e)
        return INVALID_HANDLE;
    handle32 h = handle_table_put(t->handles, e, type_tag & 0x3F);
    return h;
}

entity *entity_table_get(entity_table *t, handle32 h)
{
    if (!t)
        return NULL;
    return (entity *)handle_table_get(t->handles, h);
}

void entity_table_release(entity_table *t, handle32 h)
{
    if (!t)
        return;
    if (!handle_is_valid(t->handles, h))
        return;

    entity *e = (entity *)handle_table_get(t->handles, h);
    if (e)
    {
        /* detach from parent if attached */
        if (handle_is_valid(t->handles, e->parent))
        {
            entity_detach(t, h);
        }

        /* release children handles (do not recursively free their children here) */
        for (int i = 0; i < e->children.length; ++i)
        {
            handle32 child_h = e->children.data[i];
            if (handle_is_valid(t->handles, child_h))
            {
                /* release child handle (this will free the child entity) */
                handle_table_release(t->handles, child_h);
            }
        }

        entity_free_shallow(e);
        free(e);
    }

    handle_table_release(t->handles, h);
}

entity *entity_table_find_by_uuid(entity_table *t, u64 uuid)
{
    if (!t)
        return NULL;
    u16 cap = handle_table_capacity(t->handles);
    for (u16 i = 0; i < cap; ++i)
    {
        if (!handle_table_slot_active(t->handles, i))
            continue;
        entity *e = (entity *)handle_table_slot_ptr(t->handles, i);
        if (e && e->uuid == uuid)
            return e;
    }
    return NULL;
}

bool entity_attach(entity_table *t, handle32 parent_h, handle32 child_h)
{
    if (!t)
        return false;
    if (!handle_is_valid(t->handles, parent_h))
        return false;
    if (!handle_is_valid(t->handles, child_h))
        return false;

    entity *parent = entity_table_get(t, parent_h);
    entity *child = entity_table_get(t, child_h);
    if (!parent || !child)
        return false;

    if (handle_is_valid(t->handles, child->parent))
    {
        entity_detach(t, child_h);
    }

    child->parent = parent_h;
    (void)vec_push(&parent->children, child_h);
    /* If both are full entities with physics bodies, create a weld joint */
    if (parent->type == ENTITY_TYPE_FULL && child->type == ENTITY_TYPE_FULL)
    {
        physics_body_t pa = parent->payload.full.body_phys;
        physics_body_t cb = child->payload.full.body_phys;
        if (pa && cb)
        {
            physics_world_t pw = NULL;
            /* prefer explicit parent_room stored on the entity */
            if (parent->payload.full.parent_room)
                pw = parent->payload.full.parent_room->physics_world;
            else
            {
                LOG_ERROR("entity_attach: parent entity '%s' has no parent_room or physics world",
                          parent->name ? parent->name : "<unnamed>");
                return false;
            }
            if (pw)
            {
                physics_joint_t j = physics_create_weld_joint(pw, pa, cb);
                if (j)
                    child->payload.full.parent_joint = j;
            }
        }
    }
    return true;
}

bool entity_detach(entity_table *t, handle32 child_h)
{
    if (!t)
        return false;
    if (!handle_is_valid(t->handles, child_h))
        return false;

    entity *child = entity_table_get(t, child_h);
    if (!child)
        return false;

    if (!handle_is_valid(t->handles, child->parent))
        return true; /* already detached */

    handle32 parent_h = child->parent;
    entity *parent = entity_table_get(t, parent_h);
    if (!parent)
    {
        child->parent = INVALID_HANDLE;
        return true;
    }

    /* remove child_h from parent's children vec */
    for (int i = 0; i < parent->children.length; ++i)
    {
        handle32 cur = parent->children.data[i];
        if (handle_to_u32(cur) == handle_to_u32(child_h))
        {
            vec_splice(&parent->children, i, 1);
            break;
        }
    }

    child->parent = INVALID_HANDLE;
    /* if child had a parent_joint, destroy it */
    if (child->type == ENTITY_TYPE_FULL && child->payload.full.parent_joint)
    {
        physics_world_t pw = NULL;
        if (child->payload.full.parent_room)
            pw = child->payload.full.parent_room->physics_world;
        else
        {
            LOG_ERROR("entity_detach: child entity '%s' has no parent_room or physics world",
                      child->name ? child->name : "<unnamed>");
            return false;
        }
        if (pw)
        {
            physics_destroy_joint(pw, child->payload.full.parent_joint);
        }
        child->payload.full.parent_joint = NULL;
    }
    return true;
}

/* Convenience constructors */
handle32 entity_create_full(entity_table *t, const char *name, room body, room *parent_room, unsigned layer_index,
                            u64 collision_mask)
{
    if (!t)
    {
        if (!g_entity_table)
            g_entity_table = entity_table_create(1024);
        t = g_entity_table;
    }
    if (!t)
        return INVALID_HANDLE;

    entity *e = calloc(1, sizeof(*e));

    if (!e)
    {
        LOG_ERROR("allocation failed for a full entity");
        return INVALID_HANDLE;
    }

    e->uuid = generate_uuid();
    e->type = ENTITY_TYPE_FULL;
    e->name = name ? strdup(name) : NULL;
    e->payload.full.body = body; /* copy room struct - caller must ensure deep correctness */
    e->payload.full.parent_room = parent_room;
    e->payload.full.collision_layer_mask = collision_mask;
    e->payload.full.parent_joint = NULL;
    e->parent = INVALID_HANDLE;
    vec_init(&e->children);

    /* create physics body if parent_room (explicit) has a physics world */
    room *rptr = NULL;
    if (parent_room)
        rptr = parent_room;
    else
    {
        LOG_ERROR("entity_create_full: child entity '%s' has no parent_room", e->name ? e->name : "<unnamed>");
        return INVALID_HANDLE;
    }

    if (rptr && body.layers.length > 0 && rptr->physics_world)
    {
        unsigned use_layer = (layer_index == (unsigned)UINT_MAX) ? 0 : layer_index;
        e->payload.full.body_phys =
            physics_create_body_for_full_entity(rptr->physics_world, rptr, use_layer, e, collision_mask);
        if (!e->payload.full.body_phys)
        {
            /* physics body creation failed (detached blocks or other error). Clean up and return failure */
            entity_free_shallow(e);
            free(e);
            return INVALID_HANDLE;
        }
    }

    return entity_table_put(t, e, 0);
}