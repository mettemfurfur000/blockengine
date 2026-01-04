#include "include/level.h"
#include "include/general.h"
#include "include/handle.h"
#include "include/logging.h"
#include "include/uuid.h"
#include "include/vars.h"

#include "include/flags.h"
#include <stdarg.h>
#include <stdio.h>

static inline handle32 var_handle_from_u64(u64 v)
{
    return handle_from_u32((u32)v);
}

static inline u64 u64_from_var_handle(handle32 h)
{
    return (u64)handle_to_u32(h);
}

/* Create a new blob in the pool and return the handle. The blob memory is
   allocated and the contents copied from `vars`. Returns INVALID_HANDLE on
   failure. */
static handle32 var_table_alloc_blob(var_handle_table *pool, blob vars)
{
    if (!pool || !pool->table)
        return INVALID_HANDLE;

    blob *b = calloc(1, sizeof(blob));
    if (!b)
        return INVALID_HANDLE;
    b->ptr = calloc(vars.size ? vars.size : 1, 1);
    if (!b->ptr)
    {
        SAFE_FREE(b);
        return INVALID_HANDLE;
    }
    if (vars.size && vars.ptr)
        memcpy(b->ptr, vars.ptr, vars.size);
    b->size = vars.size;

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

u8 block_set_id(layer *l, u16 x, u16 y, u64 id)
{
    assert(l != NULL);
    if (x >= l->width || y >= l->height)
        return FAIL;

    void *ptr = BLOCK_ID_PTR(l, x, y);

    switch (l->block_size)
    {
    case 1:
        *(u8 *)ptr = id;
        break;
    case 2:
        *(u16 *)ptr = id;
        break;
    case 4:
        *(u32 *)ptr = id;
        break;
    case 8:
        *(u64 *)ptr = id;
        break;
    default:
        LOG_ERROR("FATAL: layer cannot have block size width %d bytes", l->block_size);
        return FAIL;
    }

    return SUCCESS;
}

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
        LOG_ERROR("FATAL: layer cannot have block size width %d bytes", l->block_size);
        return FAIL;
    }

    return SUCCESS;
}

u32 block_get_vars_index(layer *l, u16 x, u16 y)
{
    assert(l);

    if (x >= l->width || y >= l->height)
        return FAIL;

    if (l->var_pool.table == NULL)
        return 0;

    u32 var_index = 0;
    memcpy((u8 *)&var_index, BLOCK_ID_PTR(l, x, y) + l->block_size, sizeof(handle32));

    return var_index;
}

void block_var_index_set(layer *l, u16 x, u16 y, u32 index)
{
    assert(l);
    assert(l->blocks);
    if (FLAG_GET(l->flags, LAYER_FLAG_STATIC))
        return;
    if (x >= l->width || y >= l->height)
        return;
    if (l->var_pool.table == NULL)
        return;

    memcpy(BLOCK_ID_PTR(l, x, y) + l->block_size, (u8 *)&index, sizeof(handle32));
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

    u64 packed = 0;
    memcpy((u8 *)&packed, BLOCK_ID_PTR(l, x, y) + l->block_size, sizeof(handle32));

    if (packed == 0)
        return FAIL;

    handle32 vh = var_handle_from_u64(packed);
    blob *b = (blob *)handle_table_get(l->var_pool.table, vh);

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

    u64 packed = 0;
    memcpy((u8 *)&packed, BLOCK_ID_PTR(l, x, y) + l->block_size, sizeof(handle32));

    if (packed != 0)
    {
        handle32 vh = var_handle_from_u64(packed);
        var_table_free_handle(&l->var_pool, vh);
    }

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

    /* Check existing handle */
    u64 packed = 0;
    memcpy((u8 *)&packed, BLOCK_ID_PTR(l, x, y) + l->block_size, sizeof(handle32));

    if (packed != 0)
    {
        handle32 vh = var_handle_from_u64(packed);
        blob *existing = (blob *)handle_table_get(l->var_pool.table, vh);
        if (existing && existing->size >= vars.size)
        {
            existing->size = vars.size;
            memcpy(existing->ptr, vars.ptr, vars.size);
            return SUCCESS;
        }

        /* free existing and allocate new below */
        var_table_free_handle(&l->var_pool, vh);
    }

    handle32 newh = var_table_alloc_blob(&l->var_pool, vars);
    if (newh.index == INVALID_HANDLE_INDEX)
    {
        LOG_ERROR("Failed to create a new var");
        return FAIL;
    }

    u64 store = u64_from_var_handle(newh);
    memcpy(BLOCK_ID_PTR(l, x, y) + l->block_size, (u8 *)&store, sizeof(handle32));

    return SUCCESS;
}

u8 block_move(layer *l, u16 x, u16 y, u32 dx, u32 dy)
{
    assert(l);

    u32 dest_x = x + dx;
    u32 dest_y = y + dy;
    if (x >= l->width || y >= l->height)
        return FAIL;
    if (dest_x >= l->width || dest_y >= l->height)
        return FAIL;

    u64 id_dest = 0;
    u8 *dest_ptr = BLOCK_ID_PTR(l, dest_x, dest_y);
    u8 *src_ptr = BLOCK_ID_PTR(l, x, y);

    memcpy((u8 *)&id_dest, dest_ptr, l->block_size);
    if (id_dest != 0)
        return FAIL;

    memcpy(dest_ptr, src_ptr, l->block_size + sizeof(handle32));
    memset(src_ptr, 0, l->block_size + sizeof(handle32));

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
    // l->blocks = calloc(l->width * l->width, l->block_size + sizeof(handle32));

    if (FLAG_GET(l->flags, LAYER_FLAG_USE_VARS))
    {
        l->var_pool.table = handle_table_create(256); /* default capacity */
        // l->var_pool.type_tag = 1;                     /* choose tag 1 for vars */
    }

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

void bprintf(layer *l, const u64 character_block_id, u32 orig_x, u32 orig_y, u32 length_limit, const char *format, ...)
{
    assert(l);
    if (FLAG_GET(l->flags, LAYER_FLAG_STATIC))
        return;
    assert(l->blocks);

    char buffer[1024] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);

    char *ptr = buffer;
    int x = orig_x;
    int y = orig_y;

    blob vars_space = l->registry->resources.data[character_block_id].vars_sample;

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
            block_set_id(l, x, y, character_block_id);

            blob *vars = NULL;
            if (block_get_vars(l, x, y, &vars) != SUCCESS)
            {
                block_copy_vars(l, x, y, vars_space);
                if (block_get_vars(l, x, y, &vars) != SUCCESS)
                {
                    LOG_ERROR("bprintf Failed create a variable");
                    va_end(args);
                    return;
                }
            }

            if (var_set_u8(vars, 'v', c) != SUCCESS)
            {
                LOG_WARNING("bprintf Failed to set a u8 var for char block at %d:%d", x, y);
                // va_end(args);
                // return;
            }
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

    va_end(args);
}