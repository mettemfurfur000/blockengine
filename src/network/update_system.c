#include "include/update_system.h"
#include "include/endianless.h"

// // puts a new var to the dest pointer, returns a next spot to put the next var in
// // so it can be chained together
// u8 *patch_push_var(u8 *dest, u8 offset, u8 len, u8 *value_ptr)
// {
//     assert(dest);
//     assert(value_ptr);
//     assert(len != 0);

//     dest[0] = offset;
//     dest[1] = len;
//     memcpy(dest + 2, value_ptr, len);

//     return dest + 2 + len;
// }

// void patch_apply(blob *b, u8* patch_buffer, u32 patch_size)
// {

// }

update_acc new_update_acc()
{
    update_acc acc = {};

    vec_init(&acc.block_updates_raw);

    return acc;
}

void push_block_update(update_acc *a, u16 x, u16 y, u64 id, u8 true_width)
{
    u8 update[16] = {};
    const u8 update_width = 4 + true_width;

    *(u16 *)(update) = x;
    *(u16 *)(update + 2) = x;
    memcpy(update + 4, &id, true_width);

    make_endianless(update, 2);
    make_endianless(update + 2, 2);
    make_endianless(update + 4, true_width);

    vec_pusharr(&a->block_updates_raw, update, update_width);
}

update_block read_block_update(update_acc *a, u32 index, u8 true_width)
{
    assert(index < a->block_updates_raw.length);
    update_block update = {};
    const u8 update_width = 4 + true_width;

    u8 *offset = a->block_updates_raw.data + update_width * index;

    update.x = *(u16 *)(offset);
    update.y = *(u16 *)(offset + 2);
    update.id = *(u16 *)(offset + 4);

    make_endianless((u8 *)&update.x, sizeof(update.x));
    make_endianless((u8 *)&update.y, sizeof(update.y));
    make_endianless((u8 *)&update.id, true_width);

    return update;
}