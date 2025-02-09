#include "../include/vars.h"

i32 fdesp(blob b, char letter)
{
	if (b.size == 0)
		return FAIL;

	for (int i = 0; i < b.size; i += b.ptr[i + 1] + 2)
		if (b.ptr[i] == letter)
			return i;

	return FAIL;
}

blob tag_get(blob b, char letter)
{
	i32 index = fdesp(b, letter);

	blob ret = {};

	if (index < 0)
		return ret;

	ret.ptr = b.ptr + index + 2;
	ret.size = b.ptr[index + 1];

	return ret;
}

u8 tag_delete_all(blob *b)
{
	if (b)
		return FAIL;

	SAFE_FREE(b->ptr);
	b->size = 0;

	return SUCCESS;
}

u8 tag_create(blob *b, char letter, u8 size)
{
	if (!b)
		return FAIL;

	if (fdesp(*b, letter) >= 0)
		return SUCCESS;

	int new_data_size = b->size + size + 2;

	b->ptr = realloc(b->ptr, new_data_size);

	b->ptr[b->size] = letter;
	b->ptr[b->size + 1] = size;
	memset(b->ptr + 2, 0, size);
	b->size = new_data_size;

	return SUCCESS;
}

u8 tag_delete(blob *b, char letter)
{
	u64 blob_size = b->size;
	i32 element_pos = fdesp(*b, letter);
	u32 pos_before_element = element_pos - 1;

	if (element_pos < 0)
		return FAIL;

	u16 bytes_to_skip_size = 2 + b->ptr[element_pos + 1];
	u64 new_data_size = blob_size - bytes_to_skip_size;

	if (new_data_size == 0)
	{
		SAFE_FREE(b->ptr);
		return SUCCESS;
	}

	u8 *new_data = (u8 *)calloc(new_data_size, 1);

	if (pos_before_element <= 0) // first
		memcpy(new_data, b->ptr + bytes_to_skip_size, new_data_size);
	else if (pos_before_element + bytes_to_skip_size == blob_size - 1) // last
		memcpy(new_data, b->ptr, new_data_size);
	else // somethere in middle
	{

		// copy elements before skipped element
		memcpy(new_data, b->ptr, pos_before_element);
		// cope elements after
		memcpy(new_data + pos_before_element + bytes_to_skip_size, b->ptr, new_data_size - pos_before_element);
	}

	new_data[0] = new_data_size;

	b->ptr = new_data;

	SAFE_FREE(b->ptr);

	return SUCCESS;
}

u8 tag_resize(blob *b, char letter, u8 new_size)
{
	i32 pos = fdesp(*b, letter);
	if (pos < 0)
		return FAIL;
#if 0
	u8 cur_tag_size = b->ptr[pos + 1];
	i16 diff = new_size - cur_tag_size;

	u64 blob_size = b->size;
	u64 new_data_length = blob_size + diff;
	u16 bytes_to_skip_size = 2 + b->ptr[pos + 1];

	u8 *new_data = (u8 *)calloc(new_data_length, 1);

	memcpy(new_data, b->ptr, pos);
	memcpy(new_data + pos,
		   b->ptr + pos + bytes_to_skip_size,
		   blob_size - pos - bytes_to_skip_size);

	new_data[new_data_length - new_size - 2] = letter; // <, stupit
	new_data[new_data_length - new_size - 1] = new_size;
	memcpy(new_data + new_data_length - new_size + 1, b->ptr + pos + 2, new_size);

	b->size = new_data_length;

	SAFE_FREE(b->ptr);
	b->ptr = new_data;
#endif
	u8 cur_tag_size = b->ptr[pos + 1];
	i16 diff = new_size - cur_tag_size;

	u64 blob_size = b->size;
	u64 new_data_length = blob_size + diff;
	u16 bytes_to_skip_size = 2 + cur_tag_size;

	u8 *new_data = (u8 *)calloc(new_data_length, 1);
	if (!new_data)
		return FAIL;

	// Copy data before tag
	if (pos > 0)
		memcpy(new_data, b->ptr, pos);

	// Copy data after tag
	if (pos + bytes_to_skip_size < blob_size)
		memcpy(new_data + pos + 2 + new_size,
			   b->ptr + pos + bytes_to_skip_size,
			   blob_size - pos - bytes_to_skip_size);

	// Write new tag header
	new_data[pos] = letter;
	new_data[pos + 1] = new_size;

	// Copy tag content
	memcpy(new_data + pos + 2,
		   b->ptr + pos + 2,
		   (new_size < cur_tag_size) ? new_size : cur_tag_size);

	SAFE_FREE(b->ptr);
	b->ptr = new_data;
	b->size = new_data_length;

	return SUCCESS;
}

// utils

i32 ensure_tag(blob *b, const int letter, const int needed_size)
{
	// if tag is not existink yet, create it
	if (b->ptr == 0 || fdesp(*b, letter) < 0)
		if (tag_create(b, letter, needed_size) == FAIL) // failed to create
			return FAIL;
		else
			return fdesp(*b, letter); // created, returning the index
	else
		;

	i32 old_pos = fdesp(*b, letter); // it exists

	if (TAG_ELEMENT_SIZE(b->ptr, old_pos) < needed_size) // if size is smaller than needed, resize
		if (tag_resize(b, letter, needed_size) == FAIL)
			return FAIL;
		else
			return fdesp(*b, letter);
	else
		;

	return old_pos;
}

// will affect src
i32 data_set_num_endianless(blob *b, char letter, void *src, int size)
{
	if (!src || !b)
		return FAIL;

	int pos = ensure_tag(b, letter, size);
	if (pos < 0)
		return FAIL;

	if (make_endianless(src, size) == FAIL)
		return FAIL;

	memcpy(TAG_ELEMENT_VALUE(b->ptr, pos), src, size);

	return SUCCESS;
}

// will affect src
int data_get_num_endianless(blob b, char letter, void *dest, int size)
{
	if (!dest)
		return FAIL;

	int pos = fdesp(b, letter);
	if (pos < 0)
		return FAIL;

	u32 tag_size = TAG_ELEMENT_SIZE(b.ptr, pos);

	if (tag_size > size) // not enough space in dest ptr
		return FAIL;

	memcpy(dest, TAG_ELEMENT_VALUE(b.ptr, pos), tag_size);

	if (make_endianless(dest, tag_size) == FAIL)
		return FAIL;

	return SUCCESS;
}

// set

u8 tag_set_str(blob *b, char letter, const u8 *src, u32 size)
{
	if (!b || !src || size <= 0)
		return FAIL;

	int pos = ensure_tag(b, letter, size);
	if (pos < 0)
		return FAIL;

	memcpy(TAG_ELEMENT_VALUE(b->ptr, pos), src, size);

	return SUCCESS;
}

u8 tag_set_u(blob *b, char letter, u64 value, u8 byte_len)
{
	return data_set_num_endianless(b, letter, &value, byte_len) == FAIL;
}

u8 tag_set_i(blob *b, char letter, i64 value, u8 byte_len)
{
	return data_set_num_endianless(b, letter, &value, byte_len) == FAIL;
}

// get

u8 tag_get_str(blob b, char letter, u8 *dest, u32 size)
{
	if (!dest)
		return FAIL;

	int pos = fdesp(b, letter);
	if (pos < 0)
		return FAIL;

	int element_size = TAG_ELEMENT_SIZE(b.ptr, pos);
	int minsize = min(size, element_size);

	memcpy(dest, TAG_ELEMENT_VALUE(b.ptr, pos), minsize);
	dest[minsize] = '\0';

	return SUCCESS;
}

u8 tag_get_u(blob b, char letter, u64 *dest, u8 byte_len)
{
	return data_get_num_endianless(b, letter, dest, byte_len);
}

u8 tag_get_i(blob b, char letter, i64 *dest, u8 byte_len)
{
	return data_get_num_endianless(b, letter, dest, byte_len);
}