#include "include/level.h"
#include "include/general.h"
#include "include/handle.h"
#include "include/hashtable.h"
#include "include/logging.h"
#include "include/update_system.h"
#include "include/uuid.h"
#include "include/vars.h"

#include "include/flags.h"
#include "vec/src/vec.h"

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
        break;
    case 2:
        *id = *(u16 *)ptr;
        break;
    case 4:
        *id = *(u32 *)ptr;
        break;
    case 8:
        *id = *(u64 *)ptr;
        break;
    default:
        LOG_ERROR("layer cannot have block size width %d bytes", l->block_size);
        return FAIL;
    }

    return SUCCESS;
}

u8 block_move(layer *l, u16 x, u16 y, u32 dx, u32 dy)
{
    if (dx == 0 && dy == 0)
        return SUCCESS;

    assert(l);

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

    // moves handle
    if (l->var_pool.table == NULL)
        return SUCCESS;

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

    spatial_grid_update(&l->spatial, x, y, old_id, id);

    update_block_push(&l->id_updates, x, y, id, l->block_size);

    return SUCCESS;
}

void block_apply_id_changes(layer *l)
{
    for (u32 i = 0; i < l->id_updates.update_count; i++)
    {
        // TODO: Updates dont actually do anything because there is no real netcode at the moment and updates are not
        // being sent around the network

        // update_block u = update_block_read(l->id_updates, l->block_size);
        // block_set_id_now(l, u.x, u.y, u.id);
    }

    vec_clear(&l->id_updates.update_stream.handle.raw.bytes);
    l->id_updates.update_count = 0;
}

//
//
//

/* Create a new blob in the pool and return the handle. The blob memory is
   allocated and the contents copied from `vars`. Returns INVALID_HANDLE on
   failure.

   Set parsed to true if blob originates from a parse function
   */
static handle32 var_table_alloc_blob(var_handle_table *pool, blob vars, bool parsed)
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
static void var_table_free_handle(var_handle_table *pool, handle32 h)
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

void block_apply_varhandle_changes(layer *l)
{
    for (u32 i = 0; i < l->var_updates.update_count; i++)
    {
        // TODO: Updates dont actually do anything because there is no real netcode at the moment and updates are not
        // being sent around the network
        // update_varhandle u = update_var_read(l->var_updates);
        // block_set_var_handle_now(l, u.x, u.y, u.h);
    }

    vec_clear(&l->var_updates.update_stream.handle.raw.bytes);
    l->var_updates.update_count = 0;
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

    update_var_push(&l->var_updates, x, y, handle);
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
    
    handle32 handle = {};
    memcpy((u8 *)&handle, BLOCK_ID_PTR(l, x, y) + l->block_size, sizeof(handle32));

    // if (handle_is_valid(l->var_pool.table, handle))
    var_table_free_handle(&l->var_pool, handle);

    // Clear the var index in the block
    memset(BLOCK_ID_PTR(l, x, y) + l->block_size, 0, sizeof(handle32));

    return SUCCESS;
}

u8 block_copy_vars(layer *l, u16 x, u16 y, blob vars)
{
    assert(l);
    if (x >= l->width || y >= l->height)
        return FAIL;

    // if (sizeof(handle32) == 0)
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

            update_component_new_push(&l->var_updates, existing, vars);
            return SUCCESS;
        }

        /* free existing and allocate new below */
        block_delete_vars(l, x, y);
    }

    // have to create a new var handle for this

    handle32 newh = var_table_alloc_blob(&l->var_pool, vars, false);
    if (newh.index == INVALID_HANDLE_INDEX)
    {
        LOG_ERROR("Failed to create a new var");
        return FAIL;
    }

    blob_ptr = handle_table_get(l->var_pool.table, newh);
    assert(blob_ptr);
    memcpy(blob_ptr->ptr, vars.ptr, vars.size);

    update_component_new_push(&l->var_updates, newh, vars);

    block_set_var_handle(l, x, y, newh);

    return SUCCESS;
}

void block_apply_var_component_changes(layer *l)
{
    if (!l->var_pool.table)
        return;

    // TODO: Updates dont actually do anything because there is no real netcode at the moment and updates are not
    // being sent around the network
    goto skip;

    for (u32 i = 0; i < l->var_component_updates.update_count; i++)
    {
        update_var_component u = update_component_read(l->var_component_updates);

        switch (u.type)
        {
        case COMPONENT_UPDATE_NEW:;
            LOG_DEBUG("COMPONENT_UPDATE_NEW %d", u.size);
            handle32 ret = handle_table_set(l->var_pool.table, u.h, u.blob);
            assert(ret.index != INVALID_HANDLE_INDEX);
            assert(ret.index == u.h.index);
            break;
        case COMPONENT_UPDATE_SET:;
            LOG_DEBUG("COMPONENT_UPDATE_SET");
            blob *b = (blob *)handle_table_get(l->var_pool.table, u.h);
            assert(b);
            assert(u.size);
            switch (u.size)
            {
            case 1:
                assert(var_set_u8(b, u.letter, *u.raw) == SUCCESS);
                break;
            case 2:
                assert(var_set_u16(b, u.letter, *(u16 *)u.raw) == SUCCESS);
                break;
            case 4:
                assert(var_set_u32(b, u.letter, *(u32 *)u.raw) == SUCCESS);
                break;
            case 8:
                assert(var_set_u64(b, u.letter, *(u64 *)u.raw) == SUCCESS);
                break;
            default:
                assert(var_set_raw(b, u.letter, (blob){.ptr = u.raw, .length = u.size}) == SUCCESS);
                break;
            }
            break;
        case COMPONENT_UPDATE_ADD:
            LOG_DEBUG("COMPONENT_UPDATE_ADD");
            blob *b2 = (blob *)handle_table_get(l->var_pool.table, u.h);
            assert(b2);
            assert(var_add(b2, u.letter, u.size) == SUCCESS);
            break;
        case COMPONENT_UPDATE_DELETE:
            LOG_DEBUG("COMPONENT_UPDATE_DELETE");
            blob *b3 = (blob *)handle_table_get(l->var_pool.table, u.h);
            assert(b3);
            assert(var_delete(b3, u.letter) == SUCCESS);
            break;
        case COMPONENT_UPDATE_RESIZE:
            LOG_DEBUG("COMPONENT_UPDATE_RESIZE");
            blob *b4 = (blob *)handle_table_get(l->var_pool.table, u.h);
            assert(b4);
            assert(var_resize(b4, u.letter, u.size) == SUCCESS);
            break;
        case COMPONENT_UPDATE_RENAME:
            break;
        }
    }
skip:

    vec_clear(&l->var_component_updates.update_stream.handle.raw.bytes);
    l->var_component_updates.update_count = 0;
}

// updates

u8 block_apply_updates(layer *l)
{
    block_apply_id_changes(l);
    block_apply_varhandle_changes(l);
    block_apply_var_component_changes(l);

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
        l->var_pool.table = handle_table_create(256); /* default capacity */
        // l->var_pool.type_tag = 1;                     /* choose tag 1 for vars */
    }

    /* Initialize update accumulator for network broadcasting */
    l->id_updates = update_acc_new();
    l->var_updates = update_acc_new();
    l->var_component_updates = update_acc_new();

    return SUCCESS;
}

u8 init_room(room *r, level *parent_level)
{
    assert(parent_level);
    assert(r);

    r->parent_level = parent_level;
    r->uuid = generate_uuid();

    vec_init(&r->layers);

    /* create a physics world for this room */
    // r->physics_world = physics_world_create();

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

    if (FLAG_GET(l->flags, LAYER_FLAG_USE_VARS))
    {
        /* Free any blobs stored in the handle table and destroy it */
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
    }

    /* Cleanup update accumulator */
    update_acc_free(&l->id_updates);
    update_acc_free(&l->var_updates);
    update_acc_free(&l->var_component_updates);

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
                block_copy_vars(l, x, y, vars_scratch);
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