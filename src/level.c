#include "../include/level.h"

// layer functions

#define BLOCK_ID_PTR(l, r, x, y) (l->blocks + ((y * r->width) + x) * l->bytes_per_block)
#define MERGE32_TO_64(a, b) (((u64)a << 32) | (u64)b)

#define LAYER_CHECKS(l)

u8 block_set_id(room *r, u32 layer_index, u32 x, u32 y, u64 id)
{
    CHECK(r, set_id)
    layer l = r->layers.data[layer_index];

    CHECK(l.blocks, set_id)
    CHECK(x >= r->width || y >= r->height, set_id)

    memcpy(BLOCK_ID_PTR((&l), r, x, y), (u8 *)&id, l.bytes_per_block);

    return SUCCESS;
}

u8 block_get_id(room *r, u32 layer_index, u32 x, u32 y, u64 *id)
{
    CHECK(r, set_id)
    layer l = r->layers.data[layer_index];

    CHECK(l.blocks, set_id)
    CHECK(x >= r->width || y >= r->height, set_id)

    memcpy((u8 *)&id, BLOCK_ID_PTR((&l), r, x, y), l.bytes_per_block);

    return SUCCESS;
}

u8 block_get_vars(room *r, u32 layer_index, u32 x, u32 y, blob *vars_out)
{
    CHECK(r, block_get_vars)
    layer l = r->layers.data[layer_index];
    CHECK(x >= r->width || y >= r->height, block_get_vars);
    CHECK(vars_out, block_get_vars)
    CHECK(l.vars, block_get_vars)

    u64 key_num = MERGE32_TO_64(x, y);
    blob key = {.ptr = (u8 *)&key_num, .size = sizeof(key_num)};
    *vars_out = get_entry(l.vars, key);

    return SUCCESS;
}

u8 block_set_vars(room *r, u32 layer_index, u32 x, u32 y, blob vars)
{
    CHECK(r, block_set_vars)
    layer l = r->layers.data[layer_index];
    CHECK(x >= r->width || y >= r->height, block_set_vars)
    CHECK(l.vars, block_set_vars)

    u64 key_num = MERGE32_TO_64(x, y);
    blob key = {.ptr = (u8 *)&key_num, .size = sizeof(key_num)};
    put_entry(l.vars, key, vars);

    return SUCCESS;
}

u8 alloc_layer(layer *l, room *parent_room)
{
    CHECK(parent_room, alloc_layer)
    CHECK(l, alloc_layer)

    l->blocks = calloc(parent_room->width * parent_room->width, l->bytes_per_block);

    if (FLAG_GET(l->flags, LAYER_FLAG_HAS_vars) && l->vars == 0)
        l->vars = alloc_table();
    return SUCCESS;
}

u8 free_layer(layer *l)
{
    CHECK(l, free_layer)
    SAFE_FREE(l->blocks);
    if (l->vars)
        free_table(l->vars);

    return SUCCESS;
}

u8 alloc_room(room *r, level *parent_level, char *name, u32 width, u32 height, u32 depth)
{
    CHECK(parent_level, alloc_room)
    CHECK(r, alloc_room)

    r->parent_level = parent_level;
    r->width = width;
    r->height = height;
    r->depth = depth;

    r->name = name;

    vec_init(&r->layers);
    vec_reserve(&r->layers, depth);

    return SUCCESS;
}

u8 free_room(room *r)
{
    CHECK(r, free_room)

    for (u32 i = 0; i < r->depth; i++)
        free_layer(&r->layers.data[i]);

    vec_deinit(&r->layers);
    SAFE_FREE(r->name);

    return SUCCESS;
}