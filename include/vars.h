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

/*
// Find Data Element Start Position
returns index of a letter

blob holds the size
if the size is 0, we just don't allocate any data and keep the pointer equal to
0 format for data: (letter)(length)[data](letter)(length)[data]...

example = "a  2  h  h  b  5  a  h  f  g  b  c  1  j 	" (not real values, just
an example) indexes =  0  1  2  3  4  5  6  7  8  9  10 11 12 13 #  %  -  -  #
%  -  -  -  -  -  #  %  -

 #  - letter u8 (like name of variable)
 %  - size of variable
"-" - actual data

vars_pos(example,'c') returns 11
vars_pos(example,'a') returns 0
vars_pos(example,'g') returns -1 (Failure)

blobs store values natively to the system
*/

i32 vars_pos(const blob b, const char letter);
void *var_offset(const blob b, const char letter);
i16 var_size(blob b, char letter);

u8 var_add(blob *b, char letter, u8 size);
u8 vars_free(blob *b);

// macros to mass-produce getters and setters

#define VAR_ACCESSOR_NAME(type, op) var_##op##_##type
#define GETTER_NAME(type) VAR_ACCESSOR_NAME(type, get)
#define SETTER_NAME(type) VAR_ACCESSOR_NAME(type, set)

#define GETTER_DEF(type) u8 GETTER_NAME(type)(blob b, char letter, type *dest);
#define GETTER_IMP(type)                                                                                               \
    u8 GETTER_NAME(type)(blob b, char letter, type *dest)                                                              \
    {                                                                                                                  \
        CHECK_PTR(dest);                                                                                               \
        int pos = vars_pos(b, letter);                                                                                    \
        if (pos < 0)                                                                                                   \
            return FAIL;                                                                                               \
        *dest = *(type *)VAR_VALUE(b.ptr, pos);                                                                        \
        return SUCCESS;                                                                                                \
    }

#define SETTER_DEF(type) u8 SETTER_NAME(type)(blob * b, char letter, type value);
#define SETTER_IMP(type)                                                                                               \
    u8 SETTER_NAME(type)(blob * b, char letter, type value)                                                            \
    {                                                                                                                  \
        CHECK_PTR(b);                                                                                                  \
        int pos = vars_pos(*b, letter);                                                                                   \
        if (pos < 0)                                                                                                   \
            return FAIL;                                                                                               \
        *(type *)VAR_VALUE(b->ptr, pos) = value;                                                                       \
        return SUCCESS;                                                                                                \
    }

u8 var_set_str(blob *b, char letter, const char *str);

SETTER_DEF(u8)
SETTER_DEF(u16)
SETTER_DEF(u32)
SETTER_DEF(u64)

SETTER_DEF(i8)
SETTER_DEF(i16)
SETTER_DEF(i32)
SETTER_DEF(i64)

// get

GETTER_DEF(u8)
GETTER_DEF(u16)
GETTER_DEF(u32)
GETTER_DEF(u64)

GETTER_DEF(i8)
GETTER_DEF(i16)
GETTER_DEF(i32)
GETTER_DEF(i64)

u8 var_get_str(blob b, char letter, char **dest);

void dbg_data_layout(blob b, char *ret);

i32 data_get_num_endianless(blob b, char letter, void *dest, int size);
i32 data_set_num_endianless(blob *b, char letter, void *src, int size);

#endif