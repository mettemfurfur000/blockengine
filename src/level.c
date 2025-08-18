#include "../include/level.h"
#include "../include/uuid.h"
#include "../include/vars.h"
// #include "../include/endianless.h"
#include "../include/flags.h"
#include <stdarg.h>
#include <stdio.h>

// layer functions

#define LAYER_CHECKS(l)                                                                                                \
    CHECK_PTR(l)                                                                                                       \
    if (FLAG_GET(l->flags, LAYER_FLAG_STATIC))                                                                         \
        return FAIL;                                                                                                   \
    CHECK_PTR(l->blocks)

// object pool functions

var_holder *get_inactive(var_object_pool *pool, u32 required_size, u64 *index_out)
{
    for (u32 i = 1; i < pool->vars.length; i++)
    {
        var_holder iter = pool->vars.data[i];
        if (iter.b_ptr && !iter.active && iter.b_ptr->size >= required_size)
        {
            if (index_out)
                *index_out = (u64)i;
            return &pool->vars.data[i];
        }
    }

    return NULL;
}

var_holder *get_variable(var_object_pool *pool, u64 index)
{
    if (index == 0 || index >= pool->vars.length) // block is referencing to an invalid var
        return NULL;

    return &pool->vars.data[index];
}

var_holder new_variable(var_object_pool *pool, u32 required_size, u64 *index_out)
{
    if (required_size == 0)
    {
        LOG_ERROR("new_variable: required_size is 0, cannot create a new variable");
        return (var_holder){0};
    }

    if (pool->inactive_count != 0) // try to get an inactive var with matching size
    {
        var_holder *var = get_inactive(pool, required_size, index_out);

        if (var) // if found, reuse it
        {
            // LOG_WARNING("Reusing an inactive var...");
            var->active = true;
            var->b_ptr->size = required_size;

            pool->inactive_count--;

            // LOG_WARNING("Success...");

            return *var;
        }

        // if not, create a new one
    }

    // vars have to be allocated on the heap, so anything that exists in the engine and points at
    // the var and hopes that it exist will know fur sure it eixsts, even if vec_push expands the
    // vector to get more space for holders and shifts all fuycking objects god knows where

    var_holder new = {.active = true, .b_ptr = calloc(1, sizeof(blob))};

    new.b_ptr->ptr = calloc(required_size, 1);
    new.b_ptr->size = required_size;

    (void)vec_push(&pool->vars, new);
    if (index_out)
        *index_out = (u64)(pool->vars.length - 1);

    return new;
}

void remove_variable(var_object_pool *pool, var_holder *h)
{
    h->active = false;
    pool->inactive_count++;

    // LOG_DEBUG("marking as inactive: %p : len %d", h->b_ptr, h->b_ptr->length);
}

u8 block_set_id(layer *l, u32 x, u32 y, u64 id)
{
    LAYER_CHECKS(l)
    CHECK(x >= l->width || y >= l->height)

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

u8 block_get_id(layer *l, u32 x, u32 y, u64 *id)
{
    LAYER_CHECKS(l)
    if (x >= l->width || y >= l->height)
        return FAIL;
    CHECK_PTR(id)

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

u64 block_get_vars_index(layer *l, u32 x, u32 y)
{
    LAYER_CHECKS(l)

    u64 var_index = 0;
    memcpy((u8 *)&var_index, BLOCK_ID_PTR(l, x, y) + l->block_size, l->var_index_size);

    return var_index;
}

void block_var_index_set(layer *l, u32 x, u32 y, u64 index)
{
    CHECK_PTR_NORET(l)
    if (FLAG_GET(l->flags, LAYER_FLAG_STATIC))
        return;
    CHECK_PTR_NORET(l->blocks)

    memcpy(BLOCK_ID_PTR(l, x, y) + l->block_size, (u8 *)&index, l->var_index_size);
}

u8 block_get_vars(layer *l, u32 x, u32 y, blob **vars_out)
{
    LAYER_CHECKS(l)
    CHECK_PTR(vars_out)
    CHECK(x >= l->width || y >= l->height)

    if (l->var_index_size == 0)
        return SUCCESS;

    u64 var_index = 0;
    memcpy((u8 *)&var_index, BLOCK_ID_PTR(l, x, y) + l->block_size, l->var_index_size);

    if (var_index == 0)
        return FAIL;

    var_holder *h = get_variable(&l->var_pool, var_index);

    if (h)
        *vars_out = h->b_ptr;
    else
    {
        LOG_ERROR("Failed to lookup a var %d at %d:%d", var_index, x, y);
        *vars_out = NULL;
        return FAIL;
    }

    return SUCCESS;
}

u8 block_delete_vars(layer *l, u32 x, u32 y)
{
    LAYER_CHECKS(l)
    CHECK(x >= l->width || y >= l->height)

    if (l->var_index_size == 0)
        return SUCCESS; // nothing to delete

    u64 var_index = 0;
    memcpy((u8 *)&var_index, BLOCK_ID_PTR(l, x, y) + l->block_size, l->var_index_size);

    var_holder *h = get_variable(&l->var_pool, var_index);
    if (h)
        remove_variable(&l->var_pool, h);

    // Clear the var index in the block
    memset(BLOCK_ID_PTR(l, x, y) + l->block_size, 0, l->var_index_size);

    return SUCCESS;
}

u8 block_copy_vars(layer *l, u32 x, u32 y, blob vars)
{
    LAYER_CHECKS(l)
    CHECK(x >= l->width || y >= l->height)

    if (l->var_index_size == 0)
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

    // get an existing var at that block position
    u64 var_index = 0;
    memcpy((u8 *)&var_index, BLOCK_ID_PTR(l, x, y) + l->block_size, l->var_index_size);
    var_holder *h = get_variable(&l->var_pool, var_index);

    if (h)
    {
        // use if fits
        if (h->b_ptr->size >= vars.size)
        {
            h->b_ptr->size = vars.size;
            memcpy(h->b_ptr->ptr, vars.ptr, vars.size);
            return SUCCESS;
        }

        remove_variable(&l->var_pool, h); // didn fit, remove and mak a new one
    }

    var_holder new = new_variable(&l->var_pool, vars.size, &var_index);

    if (new.b_ptr == NULL)
    {
        LOG_ERROR("Failed to create a new var");
        return FAIL;
    }

    memcpy(new.b_ptr->ptr, vars.ptr, vars.size);
    new.b_ptr->size = vars.size;

    memcpy(BLOCK_ID_PTR(l, x, y) + l->block_size, (u8 *)&var_index, l->var_index_size);

    return SUCCESS;
}

u8 block_move(layer *l, u32 x, u32 y, u32 dx, u32 dy)
{
    LAYER_CHECKS(l)

    u32 dest_x = x + dx;
    u32 dest_y = y + dy;
    CHECK(x >= l->width || y >= l->height)
    if (dest_x >= l->width || dest_y >= l->height)
        return FAIL;

    u64 id_dest = 0;
    u8 *dest_ptr = BLOCK_ID_PTR(l, dest_x, dest_y);
    u8 *src_ptr = BLOCK_ID_PTR(l, x, y);

    memcpy((u8 *)&id_dest, dest_ptr, l->block_size);
    if (id_dest != 0)
        return FAIL;

    memcpy(dest_ptr, src_ptr, l->block_size + l->var_index_size);
    memset(src_ptr, 0, l->block_size + l->var_index_size);

    return SUCCESS;
}

// init functions

u8 init_layer(layer *l, room *parent_room)
{
    CHECK_PTR(parent_room)
    CHECK_PTR(l)

    l->parent_room = parent_room;
    l->uuid = generate_uuid();

    CHECK(l->width == 0 || l->height == 0)
    CHECK((l->block_size + l->var_index_size) == 0)
    l->total_bytes_per_block = l->block_size + l->var_index_size;

    l->blocks = calloc(l->width * l->height, l->block_size + l->var_index_size);

    if (l->blocks == NULL)
    {
        LOG_ERROR("Failed to allocate memory for blocks");
        return FAIL;
    }
    // l->blocks = calloc(l->width * l->width, l->block_size + l->var_index_size);

    if (FLAG_GET(l->flags, LAYER_FLAG_HAS_VARS))
    {
        vec_init(&l->var_pool.vars);
        l->var_pool.inactive_count = 0;
        l->var_pool.inactive_limit = 16;
        // push an empty var so that the first var is always at index 1
        (void)new_variable(&l->var_pool, 1, NULL);
    }

    return SUCCESS;
}

u8 init_room(room *r, level *parent_level)
{
    CHECK_PTR(parent_level)
    CHECK_PTR(r)

    r->parent_level = parent_level;
    r->uuid = generate_uuid();

    vec_init(&r->layers);

    CHECK_PTR(r->name)

    return SUCCESS;
}

u8 init_level(level *lvl)
{
    CHECK_PTR(lvl)

    vec_init(&lvl->rooms);
    vec_init(&lvl->registries);

    lvl->uuid = generate_uuid();

    CHECK_PTR(lvl->name)

    return SUCCESS;
}

// free functions

u8 free_layer(layer *l)
{
    CHECK_PTR(l)
    SAFE_FREE(l->blocks);
    if (FLAG_GET(l->flags, LAYER_FLAG_HAS_VARS))
    {
        /* Free any allocated blob buffers inside the var holders */
        for (u32 i = 0; i < l->var_pool.vars.length; i++)
        {
            var_holder *h = &l->var_pool.vars.data[i];
            if (h && h->b_ptr)
            {
                SAFE_FREE(h->b_ptr->ptr);
                SAFE_FREE(h->b_ptr);
            }
        }

        vec_deinit(&l->var_pool.vars);
    }

    l->uuid = 0;

    return SUCCESS;
}

u8 free_room(room *r)
{
    CHECK_PTR(r)

    for (u32 i = 0; i < r->layers.length; i++)
        free_layer(r->layers.data[i]);

    vec_deinit(&r->layers);
    SAFE_FREE(r->name);

    r->uuid = 0;

    return SUCCESS;
}

u8 free_level(level *lvl)
{
    CHECK_PTR(lvl)

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

    lvl->name = strdup(name);
    init_level(lvl);

    return lvl;
}

room *room_create(level *parent, const char *name, u32 w, u32 h)
{
    room *r = calloc(1, sizeof(room));

    r->name = strdup(name);
    r->width = w;
    r->height = h;
    r->parent_level = parent;

    init_room(r, parent);

    (void)vec_push(&parent->rooms, r);

    return r;
}

layer *layer_create(room *parent, block_registry *registry_ref, u8 bytes_per_block, u8 bytes_per_index, u8 flags)
{
    layer *l = calloc(1, sizeof(layer));

    l->registry = registry_ref;
    l->block_size = bytes_per_block;
    l->var_index_size = bytes_per_index;
    l->total_bytes_per_block = bytes_per_block + bytes_per_index;
    l->width = parent->width;
    l->height = parent->height;
    l->flags = flags;

    init_layer(l, parent);

    (void)vec_push(&parent->layers, l);

    return l;
}

void bprintf(layer *l, const u64 character_block_id, u32 orig_x, u32 orig_y, u32 length_limit, const char *format, ...)
{
    CHECK_PTR_NORET(l)
    if (FLAG_GET(l->flags, LAYER_FLAG_STATIC))
        return;
    CHECK_PTR_NORET(l->blocks)

    char buffer[1024] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);

    char *ptr = buffer;
    int x = orig_x;
    int y = orig_y;

    blob vars_space = l->registry->resources.data[character_block_id].vars_sample;

    while (*ptr != 0)
    {
        block_set_id(l, x, y, character_block_id);
        blob *vars = NULL;
        if (block_get_vars(l, x, y, &vars) != SUCCESS)
        {
            block_copy_vars(l, x, y, vars_space);
            if (block_get_vars(l, x, y, &vars) != SUCCESS)
            {
                LOG_ERROR("bprintf Failed create a variable");
                return;
            }
        }

        if (var_set_u8(vars, 'v', iscntrl(*ptr) ? ' ' : *ptr))
        {
            LOG_ERROR("bprintf Failed to set a u8 for character block in a loop");
            return;
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