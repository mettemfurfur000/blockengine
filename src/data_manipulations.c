#ifndef DATA_MANIP_H
#define DATA_MANIP_H 1

#include "memory_control_functions.c"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

// 1 byte for size
// next 1-256 bytes for custom user data
// format for data: ..(letter)(length)[data](letter)(length)[data]..

// Find Data Element Start Position (nice naming)
// returns index of letter
//
// example = "a 2 hh b 5 ahfgb c 1 j \0"
//            ^ - 1            ^ - 12
//
// fdesp(example,'c') returns 12
// dfesp(example,'g') returns 0 (Failure)

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
    int data_size = 1;
    if (data)
        data_size += data[0];

    int new_data_size = data_size + size + 2;

    if (new_data_size > 255)
        return FAIL;

    byte *new_data = (byte *)calloc(new_data_size, sizeof(byte));

    if (data)
        memcpy(new_data, data, data_size);

    new_data[data_size] = letter;
    new_data[data_size + 1] = size;
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
        return SUCCESS;

    short data_size = data[0] + 1;
    short element_pos = fdesp(data, letter);

    if (!element_pos)
        return SUCCESS;

    byte skipped_bytes_size = 2 + data[element_pos + 1];
    short new_data_size = data_size - skipped_bytes_size;

    if(new_data_size == 0)
    {
        free(data);
        *data_ptr = 0;
        return SUCCESS;
    }

    byte *new_data = (byte *)calloc(new_data_size, sizeof(byte));

    if (element_pos == 1)
    {
        // first
        memcpy(new_data,
               data + skipped_bytes_size + 1,
               new_data_size);
    }
    else if (element_pos == new_data_size)
    {
        // last
        memcpy(new_data, data, new_data_size);
    }
    else
    {
        memcpy(new_data, data, element_pos);
        memcpy(new_data + element_pos + skipped_bytes_size, data, new_data_size - element_pos);
    }

    *data_ptr = new_data;

    free(data);

    return SUCCESS;
}

// set

int data_set(byte *data, char letter, byte *src, int size)
{
    byte *el_dest = data_get_ptr(data, letter);
    if (!el_dest)
        return FAIL;

    memcpy(el_dest, src, size);

    return SUCCESS;
}

int data_set(byte *data, char letter, int value)
{
    byte *el_dest = data_get_ptr(data, letter);
    if (!el_dest)
        return FAIL;

    *el_dest = (int)htonl(value);

    return SUCCESS;
}

int data_set(byte *data, char letter, short value)
{
    byte *el_dest = data_get_ptr(data, letter);
    if (!el_dest)
        return FAIL;

    *el_dest = (short)htons(value);

    return SUCCESS;
}

int data_set(byte *data, char letter, byte value)
{
    byte *el_dest = data_get_ptr(data, letter);
    if (!el_dest)
        return FAIL;

    *el_dest = value;

    return SUCCESS;
}

// get

int data_get(byte *data, char letter, byte *dest, int size)
{
    byte *el_src = data_get_ptr(data, letter);
    if (!el_src)
        return FAIL;

    memcpy(dest, el_src, size);

    return SUCCESS;
}

int data_get(byte *data, char letter, int *dest)
{
    byte *el_src = data_get_ptr(data, letter);
    if (!el_src)
        return FAIL;

    *dest = ntohl(*(int*)el_src);

    return SUCCESS;
}

int data_get(byte *data, char letter, short *dest)
{
    byte *el_src = data_get_ptr(data, letter);
    if (!el_src)
        return FAIL;

    *dest = ntohs(*(short*)el_src);

    return SUCCESS;
}

int data_get(byte *data, char letter, byte *dest)
{
    byte *el_src = data_get_ptr(data, letter);
    if (!el_src)
        return FAIL;

    *dest = *el_src;

    return SUCCESS;
}

#endif