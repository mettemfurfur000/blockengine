#include "../include/vars.h"

i32 fdesp(blob b, char letter)
{
	if (b.size == 0 || b.ptr == 0)
		return FAIL;

	VAR_FOREACH(b, if (b.ptr[i] == letter) return i;) // found

	return FAIL; // not found
}

blob var_get(blob b, char letter)
{
	blob ret = {};
	i32 index = fdesp(b, letter);

	if (index < 0)
		return ret;

	ret.ptr = VAR_VALUE(b.ptr, index);
	ret.size = VAR_SIZE(b.ptr, index);

	return ret;
}

u8 var_delete_all(blob *b)
{
	CHECK_PTR(b);

	SAFE_FREE(b->ptr);
	b->size = 0;

	return SUCCESS;
}

u8 var_push(blob *b, char letter, u8 size)
{
	CHECK_PTR(b);
	CHECK_PTR(b->ptr);
	CHECK(b->length == 0)

	if (fdesp(*b, letter) >= 0)
		return SUCCESS;

	u32 old_size = b->size;
	u32 new_data_size = old_size + size + 2; // 1 byte for letter and 1 byte for size

	b->ptr = b->ptr ? realloc(b->ptr, new_data_size) : calloc(new_data_size, 1);
	b->ptr[old_size] = letter;
	b->ptr[old_size + 1] = size;

	b->size = new_data_size;

	return SUCCESS;
}

u8 var_delete(blob *b, char letter)
{
	CHECK_PTR(b);

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

u8 var_resize(blob *b, char letter, u8 new_size)
{
	CHECK_PTR(b);

	i32 pos = fdesp(*b, letter);
	if (pos < 0)
		return FAIL;
#if 0
	u8 cur_var_size = b->ptr[pos + 1];
	i16 diff = new_size - cur_var_size;

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
	u8 cur_var_size = b->ptr[pos + 1];
	i16 diff = new_size - cur_var_size;

	u64 blob_size = b->size;
	u64 new_data_length = blob_size + diff;
	u16 bytes_to_skip_size = 2 + cur_var_size;

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
		   (new_size < cur_var_size) ? new_size : cur_var_size);

	SAFE_FREE(b->ptr);
	b->ptr = new_data;
	b->size = new_data_length;

	return SUCCESS;
}

i32 ensure_tag(blob *b, const int letter, const int needed_size)
{
	CHECK_PTR(b);
	CHECK(needed_size == 0);
	// if tag is not existink yet, create it
	if (b->ptr == 0 || fdesp(*b, letter) < 0)
		if (var_push(b, letter, needed_size) == FAIL) // failed to create
			return FAIL;
		else
			return fdesp(*b, letter); // created, returning the index
	else
		;

	i32 old_pos = fdesp(*b, letter); // it exists

	if (VAR_SIZE(b->ptr, old_pos) < needed_size) // if size is smaller than needed, resize
		if (var_resize(b, letter, needed_size) == FAIL)
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
	CHECK_PTR(b);
	CHECK_PTR(src);

	int pos = ensure_tag(b, letter, size);
	if (pos < 0)
		return FAIL;

	if (make_endianless(src, size) == FAIL)
		return FAIL;

	memcpy(VAR_VALUE(b->ptr, pos), src, size);

	return SUCCESS;
}

// will affect dest
i32 data_get_num_endianless(blob b, char letter, void *dest, int size)
{
	if (!dest)
		return FAIL;

	int pos = fdesp(b, letter);
	if (pos < 0)
		return FAIL;

	u32 var_size = VAR_SIZE(b.ptr, pos);

	if (var_size > size) // not enough space in dest ptr
		return FAIL;

	memcpy(dest, VAR_VALUE(b.ptr, pos), var_size);

	if (make_endianless(dest, var_size) == FAIL)
		return FAIL;

	return SUCCESS;
}

// info

i16 var_size(blob b, char letter)
{
	CHECK_PTR(b.ptr);
	int pos = fdesp(b, letter);
	if (pos < 0)
		return FAIL;

	return VAR_SIZE(b.ptr, pos);
}

// set

u8 var_set_str(blob *b, char letter, const char *str)
{
	CHECK_PTR(b);
	CHECK_PTR(str);
	u32 len = strlen(str);
	CHECK(len == 0);

	int pos = ensure_tag(b, letter, len + 1);
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

	int pos = fdesp(b, letter);
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
	u8 data_size = b.size;

	char buf[256] = {};

	sprintf(buf, "data_size %d\n", data_size);
	strcat(ret, buf);

	sprintf(buf, "{\n");
	strcat(ret, buf);

	u32 index = 0;
	while (index < data_size)
	{
		byte letter = b.ptr[index];
		byte size = b.ptr[index + 1];

		void *ptr = VAR_VALUE(b.ptr, index);

		if (size == 1)
			sprintf(buf, "\tu8 %c = %d;\n", letter, *(u8 *)ptr);
		else if (size == 2)
			sprintf(buf, "\tu16 %c = %d;\n", letter, *(u16 *)ptr);
		else if (size == 4)
			sprintf(buf, "\tu32 %c = %d;\n", letter, *(u32 *)ptr);
		else if (size == 8)
			sprintf(buf, "\tu64 %c = %lld;\n", letter, *(u64 *)ptr);
		else
		{
			sprintf(buf, "\tchar %c[] = %s;\n\t// u8 %c[] = { ", letter, (char *)ptr, letter);
			strcat(ret, buf);

			for (u32 i = 0; i < size; i++)
			{
				if (i != size - 1)
					sprintf(buf, "0x%02x, ", *(u8 *)(ptr + i));
				else
					sprintf(buf, "0x%02x", *(u8 *)(ptr + i));
				strcat(ret, buf);
			}
			sprintf(buf, "};\n");
		}
		strcat(ret, buf);

		index += b.ptr[index + 1] + 2;
	}

	sprintf(buf, "}\n");
	strcat(ret, buf);
}