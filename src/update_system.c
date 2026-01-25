#include "include/update_system.h"
#include "include/endianless.h"

void update_block_push(update_block_acc *a, u16 x, u16 y, u64 id, u8 true_width)
{
    assert(true_width % 2 == 0 || true_width == 1);

    const u8 update_width = 4 + true_width;
    u8 upd_buf[16] = {};

    *(u16 *)(upd_buf) = x;                // 2 bytes X
    *(u16 *)(upd_buf + 2) = y;            // 2 bytes Y
    memcpy(upd_buf + 4, &id, true_width); // N bytes ID

    make_endianless(upd_buf, 2);
    make_endianless(upd_buf + 2, 2);
    make_endianless(upd_buf + 4, true_width);

    // TODO: replace with WRITE() to a FILE* opened on memory address (with enough space)

    vec_pusharr(&a->raw_updates, upd_buf, update_width);
    a->update_count++;
}

update_block update_block_read(update_block_acc a, u32 index, u8 true_width)
{
    assert(true_width % 2 == 0 || true_width == 1);
    assert(index < a.update_count);

    const u8 update_width = 4 + true_width;
    update_block update = {};

    u8 *offset = a.raw_updates.data + update_width * index;

    update.x = *(u16 *)(offset);                         // 2 bytes X
    update.y = *(u16 *)(offset + 2);                     // 2 bytes Y
    memcpy(&update.id, (offset + 4), true_width); // N bytes ID

    make_endianless((u8 *)&update.x, sizeof(update.x));
    make_endianless((u8 *)&update.y, sizeof(update.y));
    make_endianless((u8 *)&update.id, true_width);

    return update;
}
