#include "include/update_system.h"
#include "include/general.h"
#include "include/logging.h"
#include "vec/src/vec.h"

update_accumulator update_acc_new()
{
    update_accumulator ret = {};
    return ret;
}

void update_acc_free(update_accumulator *a)
{
    vec_deinit(&a->raw_updates);
}

static inline void endianless_set_dynamic(u8 *dest, u8 offset, u8 width, u64 val)
{
    assert(width % 2 == 0 || width == 1);

    switch (width)
    {
    case sizeof(u8):
        SET_FIELD_RAW(dest, offset, u8, val);
        break;
    case sizeof(u16):
        SET_FIELD(dest, offset, u16, val);
        break;
    case sizeof(u32):
        SET_FIELD(dest, offset, u32, val);
        break;
    case sizeof(u64):
        SET_FIELD(dest, offset, u64, val);
        break;
    default:
        assert(0 && "unreachable!");
    }
}

static inline u64 endianless_get_dynamic(u8 *src, u8 offset, u8 width)
{
    assert(width % 2 == 0 || width == 1);

    switch (width)
    {
    case sizeof(u8):
        return GET_FIELD_RAW(src, offset, u8);
    case sizeof(u16):
        return GET_FIELD(src, offset, u16);
    case sizeof(u32):
        return GET_FIELD(src, offset, u32);
    case sizeof(u64):
        return GET_FIELD(src, offset, u64);
    default:
        assert(0 && "unreachable!");
    }
}

void update_block_push(update_accumulator *a, u16 x, u16 y, u64 id, u8 true_width)
{
    const u8 update_width = 4 + true_width;
    u8 upd_buf[16] = {};

    SET_FIELD(upd_buf, 0, u16, x)
    SET_FIELD(upd_buf, 2, u16, y)

    endianless_set_dynamic(upd_buf, 4, true_width, id);

    vec_pusharr(&a->raw_updates, upd_buf, update_width);
    a->update_count++;
}

update_block update_block_read(update_accumulator a, u32 index, u8 true_width)
{
    assert(true_width % 2 == 0 || true_width == 1);
    assert(index < a.update_count);

    const u8 update_width = 4 + true_width;
    update_block update = {};

    u8 *raw_upd = a.raw_updates.data + update_width * index;

    update.x = GET_FIELD(raw_upd, 0, u16);
    update.y = GET_FIELD(raw_upd, 2, u16);
    update.id = endianless_get_dynamic(raw_upd, 4, true_width);

    return update;
}

void update_var_push(update_accumulator *a, u16 x, u16 y, handle32 handle)
{
    u8 upd_buf[16] = {};

    SET_FIELD(upd_buf, 0, u16, x)
    SET_FIELD(upd_buf, 2, u16, y)
    handle32convertor c = {.h = handle};
    SET_FIELD(upd_buf, 4, u32, c.i);
    vec_pusharr(&a->raw_updates, upd_buf, sizeof(update_varhandle));

    a->update_count++;
}

update_varhandle update_var_read(update_accumulator a, u32 index)
{
    assert(index < a.update_count);

    update_varhandle update = {};

    u8 *raw_upd = a.raw_updates.data + (sizeof(update_varhandle) * index);

    update.x = GET_FIELD(raw_upd, 0, u16);
    update.y = GET_FIELD(raw_upd, 2, u16);
    handle32convertor c = {};
    c.i = GET_FIELD(raw_upd, 4, u32);
    update.h = c.h;

    return update;
}
