#ifndef DATA_MANIPULATIONS
#define DATA_MANIPULATIONS

#include <stdlib.h>
#include <string.h>
#ifdef _WIN64
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include "block_memory_control.h"

#define BLOB_ELEMENT_SIZE(blob, pos) *(blob + pos + 1)
#define BLOB_ELEMENT_VALUE(blob, pos) (blob + pos + 2)

/*
1 byte for size
the next 1-255 bytes for user data
if the size is 0, we just don't allocate any data and keep the pointer equal to 0
format for data: ...(letter)(length)[data](letter)(length)[data]...

Find Data Element Start Position (nice naming)
returns index of letter

example = "14  a  2  h  h  b  5  a  h  f  g  b  c  1  j " (not real values, just for example)
indexes =  0   1  2  3  4  5  6  7  8  9  10 11 12 13 14
		   ^   #  %  -  -  #  %  -  -  -  -  -  #  %  -

 ^  - size byte
 #  - letter byte (like name of variable)
 %  - size of variable
"-" - actual data

fdesp(example,'c') returns 12
fdesp(example,'a') returns 1
fdesp(example,'g') returns 0 (Failure)

actual array size is 15, 1 byte for size byte and others for data (no \0 symbol, this is not string)
*/

int fdesp(byte *data, char letter);
byte *data_get_ptr(byte *data, char letter);

// memory

int data_create_element(byte **data_ptr, char letter, byte size);
int data_delete_element(byte **data_ptr, char letter);
// warning: ai-generated code
int data_resize_element(block *b, char letter, byte new_size);

// set

int data_set_str(block *b, char letter, const byte *src, int size);
int data_set_i(block *b, char letter, int value);
int data_set_s(block *b, char letter, short value);
int data_set_b(block *b, char letter, byte value);

// get

int data_get_str(block *b, char letter, byte *dest, int size);
int data_get_i(block *b, char letter, int *dest);
int data_get_s(block *b, char letter, short *dest);
int data_get_b(block *b, char letter, byte *dest);

// cooler functions

int data_get_number(block *b, char letter, long long *value_out);
int data_set_number(block *b, char letter, long long value);

#endif