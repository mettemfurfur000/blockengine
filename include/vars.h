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

#define TAG_ELEMENT_SIZE(blob, pos) *(blob + pos + 1)
#define TAG_ELEMENT_VALUE(blob, pos) (blob + pos + 2)

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
*/

i32 fdesp(blob b, char letter);

// vars

blob tag_get(blob b, char letter);

// memory

u8 tag_create(blob *b, char letter, u8 size);
u8 tag_delete(blob *b, char letter);
u8 tag_delete_all(blob *b);

u8 tag_resize(blob *b, char letter, u8 new_size);

// set

u8 tag_set_str(blob *b, char letter, const u8 *src, u32 size);
u8 tag_set_u(blob *b, char letter, u64 value, u8 byte_len);
u8 tag_set_i(blob *b, char letter, i64 value, u8 byte_len);

// get

u8 tag_get_str(blob b, char letter, u8 *dest, u32 size);
u8 tag_get_u(blob b, char letter, u64 *dest, u8 byte_len);
u8 tag_get_i(blob b, char letter, i64 *dest, u8 byte_len);

#endif