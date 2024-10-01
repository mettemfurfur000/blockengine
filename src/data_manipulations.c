#include "include/data_manipulations.h"
#include "include/endianless.h"
#include "include/block_registry.h"

/*
1 byte for size
next 1-255 bytes for user data
if the size is 0, we just don't allocate any data and keep the pointer equal to 0
format for data: ...(letter)(length)[data](letter)(length)[data]...

Find Data Element Start Position (nice naming)
returns index of a letter

example = "14  a  2  h  h  b  5  a  h  f  g  b  c  1  j " (not real values, just an example)
indexes =  0   1  2  3  4  5  6  7  8  9  10 11 12 13 14
		   ^   #  %  -  -  #  %  -  -  -  -  -  #  %  -

 ^  - size byte
 #  - letter byte (like name of variable)
 %  - size of variable
"-" - actual data

fdesp(example,'c') returns 12
fdesp(example,'a') returns 1
fdesp(example,'g') returns 0 (Failure)

actual array size is 15, 1 byte for size byte and others for data (no \0 symbol, this is not a string)
*/

int fdesp(byte *data, char letter)
{
	if (!data)
		return FAIL;

	if (data[0] == 0)
		return FAIL;

	short blob_size = data[0] + 1;

	short index = 1;

	while (index < blob_size)
	{
		if (data[index] == letter)
			return index;

		index += data[index + 1] + 2;
	}

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

	int blob_size = data ? data[0] : 0;

	int new_data_size = blob_size + size + 2;

	if (new_data_size > 0xFF)
		return FAIL;

	byte *new_data = (byte *)calloc(new_data_size + 1, sizeof(byte));

	if (data)
		memcpy(new_data, data, blob_size);

	new_data[blob_size + 1] = letter;
	new_data[blob_size + 2] = size;
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

	short blob_size = data[0];
	short element_pos = fdesp(data, letter);
	short pos_before_element = element_pos - 1;

	if (!element_pos)
		return FAIL;

	short bytes_to_skip_size = 2 + data[element_pos + 1];
	short new_data_size = blob_size - bytes_to_skip_size;

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
	else if (pos_before_element + bytes_to_skip_size == blob_size)
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

int data_resize_element(block *b, char letter, byte needed_size)
{
	if (!b)
		return FAIL;

	int pos = fdesp(b->data, letter);
	if (!pos)
		return FAIL;

	short current_size = BLOB_ELEMENT_SIZE(b->data, pos);
	short diff = needed_size - current_size;

	short blob_size = b->data[0];
	short new_data_length = blob_size + diff;

	if (new_data_length > 0xFF - 1 || new_data_length < 0)
		return FAIL;

	byte *new_data = (byte *)calloc(new_data_length, sizeof(byte));

	memcpy(new_data, b->data, pos);
	memcpy(new_data + pos + 1,
		   b->data + pos + 2 + current_size,
		   blob_size - 2 - current_size);

	new_data[0] = new_data_length;

	new_data[new_data_length - needed_size - 1] = letter;
	new_data[new_data_length - needed_size] = needed_size;
	memcpy(new_data + new_data_length - needed_size + 1, BLOB_ELEMENT_VALUE(b->data, pos), needed_size);

	free(b->data);
	b->data = new_data;

	return SUCCESS;
}

// utils

int ensure_element(block *b, const int letter, const int needed_size)
{
	if (b->data == 0 || fdesp(b->data, letter) == 0)
		if (data_create_element(&b->data, letter, needed_size) == FAIL)
			return FAIL;
		else
			return fdesp(b->data, letter);
	else
		;

	int old_pos = fdesp(b->data, letter);

	if (BLOB_ELEMENT_SIZE(b->data, old_pos) < needed_size)
		if (data_resize_element(b, letter, needed_size) == FAIL)
			return FAIL;
		else
			return fdesp(b->data, letter);
	else
		;

	return old_pos;
}

// will affect src
int data_set_num_endianless(block *b, char letter, void *src, int size)
{
	int pos = ensure_element(b, letter, size);
	if (!pos)
		return FAIL;

	if (make_endianless(src, size) == FAIL)
		return FAIL;

	memcpy(BLOB_ELEMENT_VALUE(b->data, pos), src, size);

	return SUCCESS;
}

// will affect src
int data_get_num_endianless(block *b, char letter, void *dest, int size)
{
	int pos = fdesp(b->data, letter);
	if (!pos)
		return FAIL;
	if (size != BLOB_ELEMENT_SIZE(b->data, pos))
		return FAIL;

	memcpy(dest, BLOB_ELEMENT_VALUE(b->data, pos), size);
	make_endianless(dest, size);

	return SUCCESS;
}

// set

int data_set_str(block *b, char letter, const byte *src, int size)
{
	if (!b || !src || size <= 0)
		return FAIL;

	int pos = ensure_element(b, letter, size);
	if (!pos)
		return FAIL;

	memcpy(BLOB_ELEMENT_VALUE(b->data, pos), src, size);

	return SUCCESS;
}

int data_set_i(block *b, char letter, int value)
{
	if (!b || data_set_num_endianless(b, letter, &value, sizeof(value)) == FAIL)
		return FAIL;

	return SUCCESS;
}

int data_set_s(block *b, char letter, short value)
{
	if (!b || data_set_num_endianless(b, letter, &value, sizeof(value)) == FAIL)
		return FAIL;

	return SUCCESS;
}

int data_set_b(block *b, char letter, byte value)
{
	if (!b || data_set_num_endianless(b, letter, &value, sizeof(value)) == FAIL)
		return FAIL;

	return SUCCESS;
}

// get

int data_get_str(block *b, char letter, byte *dest, int size)
{
	if (!b)
		return FAIL;

	int pos = fdesp(b->data, letter);
	if (!pos)
		return FAIL;

	int element_size = BLOB_ELEMENT_SIZE(b->data, pos);
	int minsize = min(size, element_size);

	memcpy(dest, BLOB_ELEMENT_VALUE(b->data, pos), minsize);
	dest[minsize] = '\0';

	return SUCCESS;
}

int data_get_i(block *b, char letter, int *dest)
{
	if (!b)
		return FAIL;

	if (data_get_num_endianless(b, letter, dest, sizeof(*dest)) == FAIL)
		return FAIL;

	return SUCCESS;
}

int data_get_s(block *b, char letter, short *dest)
{
	if (!b)
		return FAIL;

	if (data_get_num_endianless(b, letter, dest, sizeof(*dest)) == FAIL)
		return FAIL;

	return SUCCESS;
}

int data_get_b(block *b, char letter, byte *dest)
{
	if (!b)
		return FAIL;

	if (data_get_num_endianless(b, letter, dest, sizeof(*dest)) == FAIL)
		return FAIL;

	return SUCCESS;
}

// generalized functions

int data_get_number(block *b, char letter, long long *value_out)
{
	if (!b)
		return FAIL;

	byte pos = fdesp(b->data, letter);
	if (!pos)
		return FAIL;

	byte element_size = BLOB_ELEMENT_SIZE(b->data, pos);
	byte *src = BLOB_ELEMENT_VALUE(b->data, pos);

	memcpy(value_out, src, element_size);
	if (make_endianless((byte *)value_out, element_size) == FAIL)
		return FAIL;

	return SUCCESS;
}

int data_set_number(block *b, char letter, long long value)
{
	if (!b)
		return FAIL;

	byte value_length = length(value);

	int pos = ensure_element(b, letter, value_length);
	if (!pos)
		return FAIL;

	byte *dest = BLOB_ELEMENT_VALUE(b->data, pos);

	make_endianless((byte *)&value, value_length);
	memcpy(dest, &value, value_length);

	return SUCCESS;
}