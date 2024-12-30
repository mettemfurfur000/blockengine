#include "../include/layer.h"

#define BLOCK_ID_PTR(l, x, y) (l->blocks + ((y * l->p_room->width) + x) * l->bytes_per_block)
#define MERGE32_TO_64(a, b) (((u64)a << 32) | (u64)b)

u8 set_id(layer *l, u32 x, u32 y, u64 id)
{
    if (!l || !l->p_room || !l->blocks)
        return FAIL;

    if (x >= l->p_room->width || y >= l->p_room->height)
        return FAIL;

    void *ptr = BLOCK_ID_PTR(l, x, y);
    make_endianless((u8 *)&id, sizeof(id));
    memcpy(ptr, &id, l->bytes_per_block);

    return SUCCESS;
}

u8 get_id(layer *l, u32 x, u32 y, u64 *id)
{
    if (!l || !l->p_room || !l->blocks)
        return FAIL;

    if (x >= l->p_room->width || y >= l->p_room->height)
        return FAIL;

    void *ptr = BLOCK_ID_PTR(l, x, y);
    memcpy(id, ptr, l->bytes_per_block);
    make_endianless((u8 *)id, sizeof(*id));

    return SUCCESS;
}

u8 get_tags(layer *l, u32 x, u32 y, blob *tags_out)
{
    if (!l || !l->p_room || !l->block_tag_table)
        return FAIL;
    if (x >= l->p_room->width || y >= l->p_room->height)
        return FAIL;

    u64 key_num = MERGE32_TO_64(x, y);
    blob key = {.ptr = (u8 *)&key_num, .size = sizeof(key_num)};
    *tags_out = get_entry(l->block_tag_table, key);

    return SUCCESS;
}

u8 set_tags(layer *l, u32 x, u32 y, blob tags)
{
    if (!l || !l->p_room || !l->block_tag_table)
        return FAIL;
    if (x >= l->p_room->width || y >= l->p_room->height)
        return FAIL;

    u64 key_num = MERGE32_TO_64(x, y);
    blob key = {.ptr = (u8 *)&key_num, .size = sizeof(key_num)};
    put_entry(l->block_tag_table, key, tags);

    return SUCCESS;
}