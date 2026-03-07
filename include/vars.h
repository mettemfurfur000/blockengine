#ifndef VARS_H
#define VARS_H 1

#ifdef _WIN64
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include "hashtable.h"

#define VAR_LETTER(blob, pos) *(char *)(blob + pos)
#define VAR_SIZE(blob, pos) *(u8 *)(blob + pos + 1)
#define VAR_VALUE(blob, pos) (blob + pos + 2)

#define VAR_FOREACH(blob, code)                                                                                        \
    for (u32 i = 0; i < b.size; i += b.ptr[i + 1] + 2)                                                                 \
        code

#define VARS_MAX_INDEX_ENTRIES 32

typedef struct
{
    u8 valid;
    u8 letter;
    u32 offset;
} var_index_entry;

typedef struct
{
    var_index_entry entries[VARS_MAX_INDEX_ENTRIES];
    u8 count;
} var_index;

void var_index_build(var_index *idx, const blob b);
void var_index_clear(var_index *idx);
i32 var_index_lookup(const var_index *idx, const blob b, char letter);

i32 vars_pos_fast(const blob b, const char letter, const i32 *offsets);

i32 vars_pos(const blob b, const char letter);
void *var_offset(const blob b, const char letter);
i16 var_size(blob b, char letter);

u8 var_add(blob *b, char letter, u8 size);
u8 vars_free(blob *b);
u8 var_delete(blob *b, char letter);
u8 var_resize(blob *b, char letter, u8 new_size);
u8 var_rename(blob *b, char old_letter, char new_letter);
i32 ensure_tag(blob *b, const int letter, const int needed_size);

// macros to mass-produce getters and setters

#define VAR_ACCESSOR_NAME(type, op) var_##op##_##type
#define GETTER_NAME(type) VAR_ACCESSOR_NAME(type, get)
#define SETTER_NAME(type) VAR_ACCESSOR_NAME(type, set)

#define GETTER_DEF(type) u8 GETTER_NAME(type)(blob b, char letter, type *dest);
#define GETTER_IMP(type)                                                                                               \
    u8 GETTER_NAME(type)(blob b, char letter, type *dest)                                                              \
    {                                                                                                                  \
        assert(dest);                                                                                                  \
        i32 pos = vars_pos(b, letter);                                                                                 \
        if (pos < 0)                                                                                                   \
            return FAIL;                                                                                               \
        *dest = *(type *)VAR_VALUE(b.ptr, pos);                                                                        \
        return SUCCESS;                                                                                                \
    }

#define SETTER_DEF(type) u8 SETTER_NAME(type)(blob * b, char letter, type value);
#define SETTER_IMP(type)                                                                                               \
    u8 SETTER_NAME(type)(blob * b, char letter, type value)                                                            \
    {                                                                                                                  \
        assert(b);                                                                                                     \
        i32 pos = vars_pos(*b, letter);                                                                                \
        if (pos < 0)                                                                                                   \
            return FAIL;                                                                                               \
        *(type *)VAR_VALUE(b->ptr, pos) = value;                                                                       \
        return SUCCESS;                                                                                                \
    }

u8 var_set_str(blob *b, char letter, const char *str);
u8 var_set_raw(blob *b, char letter, blob raw);

SETTER_DEF(u8)
SETTER_DEF(u16)
SETTER_DEF(u32)
SETTER_DEF(u64)

SETTER_DEF(i8)
SETTER_DEF(i16)
SETTER_DEF(i32)
SETTER_DEF(i64)

// get

u8 var_get_str(blob b, char letter, char **dest);
u8 var_get_raw(blob b, char letter, blob *dest);

GETTER_DEF(u8)
GETTER_DEF(u16)
GETTER_DEF(u32)
GETTER_DEF(u64)

GETTER_DEF(i8)
GETTER_DEF(i16)
GETTER_DEF(i32)
GETTER_DEF(i64)

#define GETTER_FAST_DEF(type) u8 var_get_##type##_fast(blob b, char letter, const i32 *offsets, type *dest);

GETTER_FAST_DEF(u8)
GETTER_FAST_DEF(u16)
GETTER_FAST_DEF(u32)
GETTER_FAST_DEF(u64)
GETTER_FAST_DEF(i8)
GETTER_FAST_DEF(i16)
GETTER_FAST_DEF(i32)
GETTER_FAST_DEF(i64)

void dbg_data_layout(blob b, char *ret);

#endif