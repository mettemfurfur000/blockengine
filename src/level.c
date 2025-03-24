#include "../include/level.h"

// layer functions

#define BLOCK_ID_PTR(l, x, y) (l->blocks + ((y * l->width) + x) * l->total_bytes_per_block)
#define MERGE32_TO_64(a, b) (((u64)a << 32) | (u64)b)

#define LAYER_CHECKS(l)

// object pool functions

var_holder *get_inactive(var_object_pool *pool, u32 required_size, u32 *index_out)
{
    for (u32 i = 1; i < pool->vars.length; i++)
    {
        var_holder iter = pool->vars.data[i];
        if (!iter.active && iter.b->size >= required_size)
        {
            if (index_out)
                *index_out = i;
            return &pool->vars.data[i];
        }
    }

    return NULL;
}

var_holder *get_variable(var_object_pool *pool, u32 index)
{
    if (index == 0 || index >= pool->vars.length) // block is referencing to an invalid var
        return NULL;

    return &pool->vars.data[index];
}

var_holder new_variable(var_object_pool *pool, u32 required_size, u32 *index_out)
{
    if (pool->inactive_count > pool->inactive_limit)
    {
        LOG_INFO("Clearing inactive vars");
        var_holder_vec_t new_vars = {};

        vec_reserve(&new_vars, pool->vars.length - pool->inactive_count);
        // iterate through all vars and filter out inactive vars
        for (u32 i = 0; i < pool->vars.length; i++)
            if (!pool->vars.data[i].active)
            {
                SAFE_FREE(pool->vars.data[i].b);
            }
            else
            {
                (void)vec_push(&new_vars, pool->vars.data[i]);
            }

        vec_deinit(&pool->vars);
        pool->vars = new_vars;
    }

    if (pool->inactive_count != 0)
    {
        LOG_INFO("Reusing an inactive var");

        var_holder *var = get_inactive(pool, required_size, index_out);
        var->active = true;
        var->b->size = required_size;

        return *var;
    }

    var_holder new = {
        .active = true,
        .b = calloc(1, sizeof(blob) + required_size),

    };

    new.b->ptr = (u8 *)(new.b) + sizeof(blob);
    (void)vec_push(&pool->vars, new);
    if (index_out)
        *index_out = pool->vars.length - 1;

    return new;
}

void remove_variable(var_object_pool *pool, var_holder *h)
{
    h->active = false;
    pool->inactive_count++;
}

u8 block_set_id(layer *l, u32 x, u32 y, u64 id)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK(x >= l->width || y >= l->height)

    memcpy(BLOCK_ID_PTR(l, x, y), (u8 *)&id, l->block_size);

    return SUCCESS;
}

u8 block_get_id(layer *l, u32 x, u32 y, u64 *id)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK(x >= l->width || y >= l->height)

    memcpy((u8 *)id, BLOCK_ID_PTR(l, x, y), l->block_size);

    return SUCCESS;
}

u8 block_get_vars(layer *l, u32 x, u32 y, blob **vars_out)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK_PTR(vars_out)
    CHECK(x >= l->width || y >= l->height)

    u64 var_index = 0;
    memcpy((u8 *)&var_index, BLOCK_ID_PTR(l, x, y) + l->block_size, l->var_index_size);

    var_holder *h = get_variable(&l->var_pool, var_index);

    if (h)
        *vars_out = h->b;
    else
    {
        *vars_out = NULL;
        return FAIL;
    }

    return SUCCESS;
}

u8 block_delete_vars(layer *l, u32 x, u32 y)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK(x >= l->width || y >= l->height)

    return SUCCESS;
}

u8 block_set_vars(layer *l, u32 x, u32 y, blob vars)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK(x >= l->width || y >= l->height)

    u32 var_index = 0;
    memcpy((u8 *)&var_index, BLOCK_ID_PTR(l, x, y) + l->block_size, l->var_index_size); // get an existing var

    var_holder *h = get_variable(&l->var_pool, var_index);
    if (!h)
    {
        // creating a new var
        var_holder new = new_variable(&l->var_pool, vars.size, &var_index);
        memcpy(new.b->ptr, vars.ptr, vars.size);
        new.b->size = vars.size;
        memcpy(BLOCK_ID_PTR(l, x, y) + l->block_size, (u8 *)&var_index, l->var_index_size);
    }
    else
    {
        h->b->size = vars.size;
        memcpy(h->b->ptr, vars.ptr, vars.size);
    }

    return SUCCESS;
}

// init functions

u8 init_layer(layer *l, room *parent_room)
{
    CHECK_PTR(parent_room)
    CHECK_PTR(l)

    l->parent_room = parent_room;

    CHECK(l->width == 0 || l->height == 0)
    CHECK((l->block_size + l->var_index_size) == 0)

    l->blocks = calloc(l->width * l->width, l->block_size + l->var_index_size);

    if (FLAG_GET(l->flags, LAYER_FLAG_HAS_VARS))
    {
        vec_init(&l->var_pool.vars);
        l->var_pool.inactive_count = 0;
        l->var_pool.inactive_limit = 256 * 16;
        // push an empty var so that the first var is always at index 1
        (void) new_variable(&l->var_pool, 1, NULL);
    }

    return SUCCESS;
}

u8 init_room(room *r, level *parent_level)
{
    CHECK_PTR(parent_level)
    CHECK_PTR(r)

    r->parent_level = parent_level;

    vec_init(&r->layers);

    CHECK_PTR(r->name)

    return SUCCESS;
}

u8 init_level(level *l)
{
    CHECK_PTR(l)

    vec_init(&l->rooms);
    vec_init(&l->registries);

    CHECK_PTR(l->name)

    return SUCCESS;
}

// free functions

u8 free_layer(layer *l)
{
    CHECK_PTR(l)
    SAFE_FREE(l->blocks);
    if (FLAG_GET(l->flags, LAYER_FLAG_HAS_VARS))
    {
        vec_deinit(&l->var_pool.vars);
    }

    return SUCCESS;
}

u8 free_room(room *r)
{
    CHECK_PTR(r)

    for (u32 i = 0; i < r->layers.length; i++)
        free_layer(&r->layers.data[i]);

    vec_deinit(&r->layers);
    SAFE_FREE(r->name);

    return SUCCESS;
}

u8 free_level(level *l)
{
    CHECK_PTR(l)

    vec_deinit(&l->rooms);
    vec_deinit(&l->registries);

    SAFE_FREE(l->name);
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

void room_create(level *parent, const char *name, u32 w, u32 h)
{
    room r = {
        .name = strdup(name),
        .width = w,
        .height = h,
        .parent_level = parent};

    init_room(&r, parent);

    (void)vec_push(&parent->rooms, r);
}

void layer_create(room *parent, block_registry *registry_ref, u8 bytes_per_block, u8 bytes_per_index, u8 flags)
{
    layer l = {
        .registry = registry_ref,
        .block_size = bytes_per_block,
        .var_index_size = bytes_per_index,
        .total_bytes_per_block = bytes_per_block + bytes_per_index,
        .width = parent->width,
        .height = parent->height,
        .flags = flags,
    };

    init_layer(&l, parent);

    (void)vec_push(&parent->layers, l);
}

void bprintf(layer *l, const u64 character_block_id, u32 orig_x, u32 orig_y, u32 length_limit, const char *format, ...)
{
    char buffer[1024] = {};
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);

    char *ptr = buffer;
    int x = orig_x;
    int y = orig_y;

    blob vars_space = {};
    var_set_u8(&vars_space, 'v', ' ');

    while (*ptr != 0)
    {
        block_set_id(l, x, y, character_block_id);
        blob *vars = NULL;
        block_get_vars(l, x, y, &vars);
        var_set_u8(vars, 'v', iscntrl(*ptr) ? ' ' : *ptr);
        block_set_vars(l, x, y, *vars);

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

        if (x > length_limit)
        {
            x = orig_x;
            y++;
        }

        ptr++;
    }

    va_end(args);
}