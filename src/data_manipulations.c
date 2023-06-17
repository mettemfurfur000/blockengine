#ifndef DATA_MANIP_H
#define DATA_MANIP_H 1

#include "memory_control_functions.c"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

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

int fdesp(byte *data, char letter)
{
    if (!data)
        return FAIL;

    if (data[0] == 0)
        return FAIL;

    short data_size = data[0] + 1;

    short index = 1;

    while (index < data_size)
    {
        // if (data[index] == 0)
        //     return FAIL;
        if (data[index] == letter)
            return index;

        index += data[index + 1] + 2;
    }

    return FAIL;
}

byte *data_get_ptr(byte *data, char letter)
{
    if (!data)
        return FAIL;

    int letter_pos = fdesp(data, letter);

    if (letter_pos)
        return data + letter_pos + 2;

    return FAIL;
}

// memory

int data_create_element(byte **data_ptr, char letter, byte size)
{
    if (!data_ptr)
        return FAIL;
    byte *data = *data_ptr;

    if (data)
        if (fdesp(data, letter))
            return SUCCESS;

    int data_size = data ? data[0] : 0;

    int new_data_size = data_size + size + 2;

    if (new_data_size > 0xFF)
        return FAIL;

    byte *new_data = (byte *)calloc(new_data_size + 1, sizeof(byte));

    if (data)
        memcpy(new_data, data, data_size);

    new_data[data_size + 1] = letter;
    new_data[data_size + 2] = size;
    new_data[0] = new_data_size;

    *data_ptr = new_data;

    if (data)
        free(data);

    return SUCCESS;
}

int data_delete_element(byte **data_ptr, char letter)
{
    if (!data_ptr)
        return FAIL;

    byte *data = *data_ptr;

    if (!data)
        return FAIL;

    short data_size = data[0];
    short element_pos = fdesp(data, letter);
    short pos_before_element = element_pos - 1;

    if (!element_pos)
        return FAIL;

    short bytes_to_skip_size = 2 + data[element_pos + 1];
    short new_data_size = data_size - bytes_to_skip_size;

    if (new_data_size == 0)
    {
        free(data);
        *data_ptr = 0;
        return SUCCESS;
    }

    byte *new_data = (byte *)calloc(new_data_size + 1, sizeof(byte));
    byte *actual_data = data + 1;
    byte *actual_new_data = new_data + 1;

    if (pos_before_element == 0)
    {
        // first
        memcpy(actual_new_data, actual_data + bytes_to_skip_size, new_data_size);
    }
    else if (pos_before_element + bytes_to_skip_size == data_size)
    {
        // last
        memcpy(actual_new_data, actual_data, new_data_size);
    }
    else
    {
        // somethere in middle
        // copy elements before skipped element
        memcpy(actual_new_data, actual_data, pos_before_element);
        // cope elements after
        memcpy(actual_new_data + pos_before_element + bytes_to_skip_size, actual_data, new_data_size - pos_before_element);
    }

    new_data[0] = new_data_size;

    *data_ptr = new_data;

    free(data);

    return SUCCESS;
}

// set

int data_set_str(byte *data, char letter, byte *src, int size)
{
    byte *el_dest = data_get_ptr(data, letter);
    if (!el_dest)
        return FAIL;

    memcpy(el_dest, src, size);

    return SUCCESS;
}

int data_set_i(byte *data, char letter, int value)
{
    byte *el_dest = data_get_ptr(data, letter);
    if (!el_dest)
        return FAIL;

    *(int *)el_dest = (int)htonl(value);

    return SUCCESS;
}

int data_set_s(byte *data, char letter, short value)
{
    byte *el_dest = data_get_ptr(data, letter);
    if (!el_dest)
        return FAIL;

    *(short *)el_dest = (short)htons(value);

    return SUCCESS;
}

int data_set_b(byte *data, char letter, byte value)
{
    byte *el_dest = data_get_ptr(data, letter);
    if (!el_dest)
        return FAIL;

    *el_dest = value;

    return SUCCESS;
}

// get

int data_get_str(byte *data, char letter, byte *dest, int size)
{
    byte *el_src = data_get_ptr(data, letter);
    if (!el_src)
        return FAIL;

    memcpy(dest, el_src, size);

    return SUCCESS;
}

int data_get_i(byte *data, char letter, int *dest)
{
    byte *el_src = data_get_ptr(data, letter);
    if (!el_src)
        return FAIL;

    *(int *)dest = ntohl(*(int *)el_src);

    return SUCCESS;
}

int data_get_s(byte *data, char letter, short *dest)
{
    byte *el_src = data_get_ptr(data, letter);
    if (!el_src)
        return FAIL;

    *(short *)dest = ntohs(*(short *)el_src);

    return SUCCESS;
}

int data_get_b(byte *data, char letter, byte *dest)
{
    byte *el_src = data_get_ptr(data, letter);
    if (!el_src)
        return FAIL;

    *dest = *el_src;

    return SUCCESS;
}

#endif