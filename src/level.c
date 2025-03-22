#include "../include/level.h"

// layer functions

#define BLOCK_ID_PTR(l, x, y) (l->blocks + ((y * l->width) + x) * l->bytes_per_block)
#define MERGE32_TO_64(a, b) (((u64)a << 32) | (u64)b)

#define LAYER_CHECKS(l)

u8 block_set_id(layer *l, u32 x, u32 y, u64 id)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK(x >= l->width || y >= l->height)

    memcpy(BLOCK_ID_PTR(l, x, y), (u8 *)&id, l->bytes_per_block);

    return SUCCESS;
}

u8 block_get_id(layer *l, u32 x, u32 y, u64 *id)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK(x >= l->width || y >= l->height)

    memcpy((u8 *)id, BLOCK_ID_PTR(l, x, y), l->bytes_per_block);

    return SUCCESS;
}

u8 block_get_vars(layer *l, u32 x, u32 y, blob *vars_out)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK_PTR(vars_out)
    CHECK(x >= l->width || y >= l->height)
    CHECK(l->vars)

    u64 key_num = MERGE32_TO_64(x, y);
    blob key = {.ptr = (u8 *)&key_num, .size = sizeof(key_num)};
    *vars_out = get_entry(l->vars, key);

    return SUCCESS;
}

u8 block_set_vars(layer *l, u32 x, u32 y, blob vars)
{
    CHECK_PTR(l)
    CHECK_PTR(l->blocks)
    CHECK_PTR(l->vars)
    CHECK(x >= l->width || y >= l->height)

    u64 key_num = MERGE32_TO_64(x, y);
    blob key = {.ptr = (u8 *)&key_num, .size = sizeof(key_num)};
    put_entry(l->vars, key, vars);

    return SUCCESS;
}

// init functions

u8 init_layer(layer *l, room *parent_room)
{
    CHECK_PTR(parent_room)
    CHECK_PTR(l)

    CHECK(l->width == 0 || l->height == 0)
    CHECK(l->bytes_per_block == 0)

    l->blocks = calloc(l->width * l->width, l->bytes_per_block);

    if (FLAG_GET(l->flags, LAYER_FLAG_HAS_VARS) && l->vars == 0)
        l->vars = alloc_table();

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
    if (l->vars)
        free_table(l->vars);

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

level *level_create(const char *name, u32 width, u32 height)
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
        .parent_level = parent
    };

    init_room(&r, parent);

    (void)vec_push(&parent->rooms, r);
}

void layer_create(room *parent, block_registry *registry_ref, u8 bytes_per_block, u8 flags)
{
    layer l = {
        .registry = registry_ref,
        .bytes_per_block = bytes_per_block,
        .width = parent->width,
        .height = parent->height,
        .flags = flags,
    };

    init_layer(&l, parent);

    (void)vec_push(&parent->layers, l);
}