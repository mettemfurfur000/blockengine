#ifndef vars_MANIPULATIONS_H
#define vars_MANIPULATIONS_H 1

#include <stdlib.h>
#include <string.h>
#ifdef _WIN64
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include "endianless.h"
#include "block_registry.h"
#include "hashtable.h"

#define VAR_ELEMENT_SIZE(blob, pos) *(blob + pos + 1)
#define VAR_ELEMENT_VALUE(blob, pos) (blob + pos + 2)

/*
Find Data Element Start Position
returns index of a letter

blob holds the size
if the size is 0, we just don't allocate any data and keep the pointer equal to 0
format for data: (letter)(length)[data](letter)(length)[data]...

example = "a  2  h  h  b  5  a  h  f  g  b  c  1  j 	" (not real values, just an example)
indexes =  0  1  2  3  4  5  6  7  8  9  10 11 12 13
		   #  %  -  -  #  %  -  -  -  -  -  #  %  -

 #  - letter u8 (like name of variable)
 %  - size of variable
"-" - actual data

fdesp(example,'c') returns 11
fdesp(example,'a') returns 0
fdesp(example,'g') returns -1 (Failure)

blobs store values natively to the system
*/

i32 fdesp(blob b, char letter);

// vars

blob var_get(blob b, char letter);

// memory

u8 var_push(blob *b, char letter, u8 size);
u8 var_delete(blob *b, char letter);
u8 var_delete_all(blob *b);

u8 var_resize(blob *b, char letter, u8 new_size);
i32 ensure_tag(blob *b, const int letter, const int needed_size);

#define VAR_ACCESSOR_NAME(type, op) var_##op##_##type
#define GETTER_NAME(type) VAR_ACCESSOR_NAME(type, get)
#define SETTER_NAME(type) VAR_ACCESSOR_NAME(type, set)

#define GETTER_DEF(type) u8 GETTER_NAME(type)(blob b, char letter, type *dest);
#define GETTER_IMP(type)                                  \
	u8 GETTER_NAME(type)(blob b, char letter, type *dest) \
	{                                                     \
		CHECK_PTR(dest);                                  \
		int pos = fdesp(b, letter);                       \
		CHECK(pos < 0);                                   \
		*dest = *(type *)VAR_ELEMENT_VALUE(b.ptr, pos);   \
		return SUCCESS;                                   \
	}

#define SETTER_DEF(type) u8 SETTER_NAME(type)(blob * b, char letter, type value);
#define SETTER_IMP(type)                                    \
	u8 SETTER_NAME(type)(blob * b, char letter, type value) \
	{                                                       \
		CHECK_PTR(b);                                       \
		int pos = ensure_tag(b, letter, sizeof(type));      \
		CHECK(pos < 0);                                     \
		*(type *)VAR_ELEMENT_VALUE(b->ptr, pos) = value;    \
		return SUCCESS;                                     \
	}

// set

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

// utils

void dbg_data_layout(blob b);

i32 data_get_num_endianless(blob b, char letter, void *dest, int size);
i32 data_set_num_endianless(blob *b, char letter, void *src, int size);

#endif