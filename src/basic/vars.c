#include "include/vars.h"
#include "include/endianless.h"

#include <stdlib.h>

i32 vars_pos(const blob b, const char letter)
{
    if (b.size == 0 || b.ptr == 0)
        return FAIL;

    /* Iterate entries safely. Each entry is laid out as:
     * [letter (1)] [size (1)] [value (size)]
     * Ensure we never read past b.size and detect malformed entries.
     */
    u32 i = 0;
    while (i + 1 < b.size)
    {
        u8 entry_size = b.ptr[i + 1];

        /* check that the claimed entry fits inside the blob */
        if ((u32)i + 2 + (u32)entry_size > b.size)
        {
            LOG_ERROR("Malformed vars blob: entry at %u claims size %u but blob size is %u", i, entry_size, b.size);
            return FAIL;
        }

        if (b.ptr[i] == (u8)letter)
            return (i32)i;

        i += (u32)entry_size + 2;
    }

    return FAIL; // not found
}

void *var_offset(const blob b, const char letter)
{
    i32 pos = vars_pos(b, letter);
    if (pos < 0)
        return NULL;

    return VAR_VALUE(b.ptr, pos);
}

i16 var_size(blob b, char letter)
{
    i32 pos = vars_pos(b, letter);
    if (pos < 0)
        return -1;

    return (i16)VAR_SIZE(b.ptr, pos);
}

u8 vars_free(blob *b)
{
    assert(b);

    if (b->ptr)
    {
        free(b->ptr);
        b->ptr = NULL;
    }
    b->size = 0;

    return SUCCESS;
}

u8 var_delete(blob *b, char letter)
{
    assert(b);

    i32 pos = vars_pos(*b, letter);
    if (pos < 0)
        return FAIL;

    u32 upos = (u32)pos;
    u32 entry_size = VAR_SIZE(b->ptr, upos);
    u32 bytes_to_skip = 2 + entry_size;

    if (bytes_to_skip > b->size)
        return FAIL; /* malformed */

    u32 new_data_size = b->size - bytes_to_skip;

    if (new_data_size == 0)
    {
        SAFE_FREE(b->ptr);
        b->size = 0;
        return SUCCESS;
    }

    u8 *new_data = (u8 *)calloc(new_data_size, 1);
    if (!new_data)
        return FAIL;

    /* copy before entry */
    if (upos > 0)
        memcpy(new_data, b->ptr, upos);

    /* copy after entry */
    if (upos + bytes_to_skip < b->size)
        memcpy(new_data + upos, b->ptr + upos + bytes_to_skip, b->size - (upos + bytes_to_skip));

    SAFE_FREE(b->ptr);
    b->ptr = new_data;
    b->size = new_data_size;

    return SUCCESS;
}

u8 var_add(blob *b, char letter, u8 size)
{
    assert(b);
    assert(size != 0);

    if (vars_pos(*b, letter) >= 0) // make sure it doesn't exist first
        return SUCCESS;

    u32 old_size = b->size;
    u32 new_data_size = old_size + size + 2; // 1 u8 for letter and 1 u8 for size

    void *new_ptr = b->ptr ? realloc(b->ptr, new_data_size) : calloc(new_data_size, 1);
    if (!new_ptr)
    {
        LOG_ERROR("Failed to allocate memory for a var: %c with size %d, final size: %d", letter, size, new_data_size);
        return FAIL;
    }

    b->ptr = new_ptr;
    b->ptr[old_size] = (u8)letter;
    b->ptr[old_size + 1] = (u8)size;

    b->size = new_data_size;

    return SUCCESS;
}

u8 var_rename(blob *b, char old_letter, char new_letter)
{
    assert(b);

    i32 pos_old = vars_pos(*b, old_letter);
    if (pos_old < 0)
        return FAIL;

    /* don't overwrite existing variable */
    if (vars_pos(*b, new_letter) >= 0)
        return FAIL;

    /* simply change header letter */
    b->ptr[(u32)pos_old] = (u8)new_letter;
    return SUCCESS;
}

u8 var_resize(blob *b, char letter, u8 new_size)
{
    assert(b);

    i32 pos_i = vars_pos(*b, letter);
    if (pos_i < 0)
        return FAIL;

    u32 pos = (u32)pos_i;
    u8 old_size = VAR_SIZE(b->ptr, pos);
    if (old_size == new_size)
        return SUCCESS;

    u32 new_blob_size = b->size - (u32)old_size + (u32)new_size;
    u8 *new_data = (u8 *)calloc(new_blob_size, 1);
    if (!new_data)
        return FAIL;

    /* copy before the entry */
    if (pos > 0)
        memcpy(new_data, b->ptr, pos);

    /* write header */
    new_data[pos] = (u8)letter;
    new_data[pos + 1] = new_size;

    /* copy value: copy min(old_size, new_size) bytes of existing data */
    u32 copy_bytes = old_size < new_size ? old_size : new_size;
    if (copy_bytes)
        memcpy(new_data + pos + 2, b->ptr + pos + 2, copy_bytes);

    /* copy data after old entry */
    u32 tail_src = pos + 2 + old_size;
    u32 tail_dst = pos + 2 + new_size;
    if (tail_src < b->size)
        memcpy(new_data + tail_dst, b->ptr + tail_src, b->size - tail_src);

    SAFE_FREE(b->ptr);
    b->ptr = new_data;
    b->size = new_blob_size;

    return SUCCESS;
}

i32 ensure_tag(blob *b, const int letter, const int needed_size)
{
    assert(b);
    assert(needed_size != 0);

    i32 pos = vars_pos(*b, (char)letter);
    if (pos < 0)
    {
        /* create */
        if (var_add(b, (char)letter, (u8)needed_size) != SUCCESS)
            return FAIL;
        return vars_pos(*b, (char)letter);
    }

    /* exists, ensure size */
    u8 cur_size = VAR_SIZE(b->ptr, (u32)pos);
    if (cur_size < (u8)needed_size)
    {
        if (var_resize(b, (char)letter, (u8)needed_size) != SUCCESS)
            return FAIL;
        return vars_pos(*b, (char)letter);
    }

    return pos;
}

// will affect src
i32 data_set_num_endianless(blob *b, char letter, void *src, int size)
{
    assert(b);
    assert(src);

    i32 pos = vars_pos(*b, letter);
    if (pos < 0)
        return FAIL;

    if (make_endianless(src, size) != SUCCESS)
        return FAIL;

    memcpy(VAR_VALUE(b->ptr, pos), src, size);

    return SUCCESS;
}

// will affect dest
i32 data_get_num_endianless(blob b, char letter, void *dest, int size)
{
    if (!dest)
        return FAIL;

    i32 pos = vars_pos(b, letter);
    if (pos < 0)
        return FAIL;

    u32 var_size = VAR_SIZE(b.ptr, pos);

    if (var_size > size) // not enough space in dest ptr
        return FAIL;

    memcpy(dest, VAR_VALUE(b.ptr, pos), var_size);

    if (make_endianless(dest, var_size) != SUCCESS)
        return FAIL;

    return SUCCESS;
}

// set

u8 var_set_str(blob *b, char letter, const char *str)
{
    assert(b);
    assert(str);
    u32 len = strlen(str);
    assert(len != 0);
    assert(len <= 0xff - 1);

    i32 pos = vars_pos(*b, letter);
    if (pos < 0)
        return FAIL;

    memcpy(VAR_VALUE(b->ptr, pos), str, len + 1);

    return SUCCESS;
}

SETTER_IMP(u8)
SETTER_IMP(u16)
SETTER_IMP(u32)
SETTER_IMP(u64)

SETTER_IMP(i8)
SETTER_IMP(i16)
SETTER_IMP(i32)
SETTER_IMP(i64)

// get

u8 var_get_str(blob b, char letter, char **dest)
{
    if (!dest)
        return FAIL;

    i32 pos = vars_pos(b, letter);
    if (pos < 0)
        return FAIL;

    *dest = (char *)VAR_VALUE(b.ptr, pos);

    return SUCCESS;
}

GETTER_IMP(u8)
GETTER_IMP(u16)
GETTER_IMP(u32)
GETTER_IMP(u64)

GETTER_IMP(i8)
GETTER_IMP(i16)
GETTER_IMP(i32)
GETTER_IMP(i64)

// utils

void dbg_data_layout(blob b, char *ret)
{
    u32 data_size = b.size;

    char buf[256];

    snprintf(buf, sizeof(buf), "data_size %u\n{\n", data_size);
    strcat(ret, buf);

    u32 index = 0;
    while (index + 1 < data_size)
    {
        u8 letter = b.ptr[index];
        u8 size = b.ptr[index + 1];

        /* guard against malformed blobs */
        if ((u32)index + 2 + (u32)size > data_size)
        {
            snprintf(buf, sizeof(buf), "\t<malformed entry at %u: size %u extends beyond blob>\n", index, size);
            strcat(ret, buf);
            break;
        }

        void *ptr = VAR_VALUE(b.ptr, index);

        if (size == 1)
            snprintf(buf, sizeof(buf), "\ti8 %c = %d;\n", letter, *(u8 *)ptr);
        else if (size == 2)
            snprintf(buf, sizeof(buf), "\ti16 %c = %d;\n", letter, *(u16 *)ptr);
        else if (size == 4)
            snprintf(buf, sizeof(buf), "\ti32 %c = %d;\n", letter, *(u32 *)ptr);
        else if (size == 8)
            snprintf(buf, sizeof(buf), "\ti64 %c = %lld;\n", letter, *(u64 *)ptr);
        else
        {
            /* print as string and hex bytes */
            snprintf(buf, sizeof(buf), "\tchar %c[] = %.*s;\n\t// u8 %c[] = { ", letter, size, (char *)ptr, letter);
            strcat(ret, buf);

            for (u32 i = 0; i < size; i++)
            {
                if (i != size - 1)
                    snprintf(buf, sizeof(buf), "0x%02x, ", *(u8 *)((u8 *)ptr + i));
                else
                    snprintf(buf, sizeof(buf), "0x%02x", *(u8 *)((u8 *)ptr + i));
                strcat(ret, buf);
            }
            snprintf(buf, sizeof(buf), "};\n");
            strcat(ret, buf);

            index += (u32)size + 2;
            continue;
        }

        strcat(ret, buf);

        index += (u32)size + 2;
    }

    strcat(ret, "}\n");
}