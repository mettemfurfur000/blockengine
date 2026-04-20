#include "include/vars.h"

#include <stdlib.h>

/*
	Vars are dynamic structures that can hold any arbitrary key-value pairs, but key can only
	be a single character, size of variables is a single byte, and payload can be anything

	When a value is 2, 4 or 8 bytes long, it will be serialized with endianless functions to work across different
	endian machines (theoretically, may not work at all...)

	Values of any other lengths are not affected. If a string is 4 bytes long, i think its still fine if its
   endianlessly shifted and reconstructed back on saves-loads.

	Vars could theoretically hold handle32 values that point to other vars, creating tree-like structures, but i havent
   tested that yet and it may not work across networks because not everything can be perfectly syncronized across
   clients. Or maybe it can, we will see!

	TODO: verify that Vars are perfectly syncronized between clients and servers, excluding the clientside overlay
   layers

*/

/* Iterate entries safely. Each entry is laid out as:
 * [letter (1)] [size (1)] [value (size)]
 * Ensure we never read past b.size and detect malformed entries.
 */

#define VARS_CACHE_SIZE 128

typedef struct
{
	const u8 *blob_ptr;
	u32 blob_size;
	char letter;
	i32 position;
} vars_cache_entry;

static vars_cache_entry vars_cache[VARS_CACHE_SIZE] = {0};
static u8 vars_cache_index = 0;

static i32 vars_pos_cached(const blob b, const char letter)
{
	if (b.size == 0 || b.ptr == 0)
		return FAIL;

	for (u8 c = 0; c < VARS_CACHE_SIZE; c++)
	{
		vars_cache_entry *e = &vars_cache[c];
		if (e->blob_ptr == b.ptr && e->blob_size == b.size && e->letter == letter)
		{
			if (e->position >= 0 && (u32)e->position < b.size)
			{
				return e->position;
			}
		}
	}

	return FAIL;
}

static void vars_pos_update_cache(const blob b, const char letter, i32 position)
{
	vars_cache_entry *e = &vars_cache[vars_cache_index];
	e->blob_ptr = b.ptr;
	e->blob_size = b.size;
	e->letter = letter;
	e->position = position;
	vars_cache_index = (vars_cache_index + 1) % VARS_CACHE_SIZE;
}

void var_index_build(var_index *idx, const blob b)
{
	idx->count = 0;
	if (b.size == 0 || b.ptr == 0)
		return;

	u32 i = 0;
	while (i + 1 < b.size && idx->count < VARS_MAX_INDEX_ENTRIES)
	{
		u8 entry_size = b.ptr[i + 1];

		if ((u32)i + 2 + (u32)entry_size > b.size)
			break;

		idx->entries[idx->count].valid = 1;
		idx->entries[idx->count].letter = b.ptr[i];
		idx->entries[idx->count].offset = i;
		idx->count++;

		i += (u32)entry_size + 2;
	}
}

void var_index_clear(var_index *idx)
{
	idx->count = 0;
	for (u8 i = 0; i < VARS_MAX_INDEX_ENTRIES; i++)
	{
		idx->entries[i].valid = 0;
	}
}

i32 var_index_lookup(const var_index *idx, const blob b, char letter)
{
	for (u8 i = 0; i < idx->count; i++)
	{
		if (idx->entries[i].valid && idx->entries[i].letter == (u8)letter)
		{
			u32 offset = idx->entries[i].offset;
			if (offset < b.size)
			{
				return (i32)offset;
			}
		}
	}
	return FAIL;
}

i32 vars_pos(const blob b, const char letter)
{
	if (b.size == 0 || b.ptr == 0)
		return FAIL;

	i32 cached = vars_pos_cached(b, letter);
	if (cached != FAIL)
		return cached;

	u32 i = 0;
	while (i + 1 < b.size)
	{
		u8 entry_size = b.ptr[i + 1];

		/* check that the claimed entry fits inside the blob */
		if ((u32)i + 2 + (u32)entry_size > b.size)
		{
			LOG_ERROR("Malformed vars blob: entry at %u claims size %u but blob size is %u", i, entry_size, b.size);
			return FAIL;
		}

		if (b.ptr[i] == (u8)letter)
		{
			vars_pos_update_cache(b, letter, (i32)i);
			return (i32)i;
		}

		i += (u32)entry_size + 2;
	}

	vars_pos_update_cache(b, letter, FAIL);
	return FAIL;
}

i32 vars_pos_fast(const blob b, const char letter, const i32 *offsets)
{
	if (b.size == 0 || b.ptr == 0)
		return FAIL;

	if (offsets)
	{
		i32 offset = offsets[(u8)letter];
		if (offset != FAIL)
		{
			if ((u32)offset + 2 <= b.size)
			{
				return offset;
			}
		}
	}

	return vars_pos(b, letter);
}

void *var_offset(const blob b, const char letter)
{
	i32 pos = vars_pos(b, letter);
	if (pos < 0)
		return NULL;

	return VAR_VALUE(b.ptr, pos);
}

i16 var_size(blob b, char letter)
{
	i32 pos = vars_pos(b, letter);
	if (pos < 0)
		return -1;

	return (i16)VAR_SIZE(b.ptr, pos);
}

u8 vars_free(blob *b)
{
	assert(b);

	if (b->ptr)
	{
		free(b->ptr);
		b->ptr = NULL;
	}
	b->size = 0;

	return SUCCESS;
}

u8 var_delete(blob *b, char letter)
{
	assert(b);

	i32 pos = vars_pos(*b, letter);
	if (pos < 0)
		return FAIL;

	u32 upos = (u32)pos;
	u32 entry_size = VAR_SIZE(b->ptr, upos);
	u32 bytes_to_skip = 2 + entry_size;

	if (bytes_to_skip > b->size)
		return FAIL; /* malformed */

	u32 new_data_size = b->size - bytes_to_skip;

	if (new_data_size == 0)
	{
		SAFE_FREE(b->ptr);
		b->size = 0;
		return SUCCESS;
	}

	u8 *new_data = (u8 *)calloc(new_data_size, 1);
	if (!new_data)
		return FAIL;

	/* copy before entry */
	if (upos > 0)
		memcpy(new_data, b->ptr, upos);

	/* copy after entry */
	if (upos + bytes_to_skip < b->size)
		memcpy(new_data + upos, b->ptr + upos + bytes_to_skip, b->size - (upos + bytes_to_skip));

	SAFE_FREE(b->ptr);
	b->ptr = new_data;
	b->size = new_data_size;

	return SUCCESS;
}

u8 var_add(blob *b, char letter, u8 size)
{
	assert(b);
	assert(size != 0);

	if (vars_pos(*b, letter) >= 0) // make sure it doesn't exist first
		return SUCCESS;

	u32 old_size = b->size;
	u32 new_data_size = old_size + size + 2; // 1 u8 for letter and 1 u8 for size

	void *new_ptr = b->ptr ? realloc(b->ptr, new_data_size) : calloc(new_data_size, 1);
	if (!new_ptr)
	{
		LOG_ERROR("Failed to allocate memory for a var: %c with size %d, final size: %d", letter, size, new_data_size);
		return FAIL;
	}

	b->ptr = new_ptr;
	b->ptr[old_size] = (u8)letter;
	b->ptr[old_size + 1] = (u8)size;

	b->size = new_data_size;

	return SUCCESS;
}

u8 var_rename(blob *b, char old_letter, char new_letter)
{
	assert(b);

	i32 pos_old = vars_pos(*b, old_letter);
	if (pos_old < 0)
		return FAIL;

	/* don't overwrite existing variable */
	if (vars_pos(*b, new_letter) >= 0)
		return FAIL;

	/* simply change header letter */
	b->ptr[(u32)pos_old] = (u8)new_letter;
	return SUCCESS;
}

u8 var_resize(blob *b, char letter, u8 new_size)
{
	assert(b);

	i32 pos_i = vars_pos(*b, letter);
	if (pos_i < 0)
		return FAIL;

	u32 pos = (u32)pos_i;
	u8 old_size = VAR_SIZE(b->ptr, pos);
	if (old_size == new_size)
		return SUCCESS;

	u32 new_blob_size = b->size - (u32)old_size + (u32)new_size;
	u8 *new_data = (u8 *)calloc(new_blob_size, 1);
	if (!new_data)
		return FAIL;

	/* copy before the entry */
	if (pos > 0)
		memcpy(new_data, b->ptr, pos);

	/* write header */
	new_data[pos] = (u8)letter;
	new_data[pos + 1] = new_size;

	/* copy value: copy min(old_size, new_size) bytes of existing data */
	u32 copy_bytes = old_size < new_size ? old_size : new_size;
	if (copy_bytes)
		memcpy(new_data + pos + 2, b->ptr + pos + 2, copy_bytes);

	/* copy data after old entry */
	u32 tail_src = pos + 2 + old_size;
	u32 tail_dst = pos + 2 + new_size;
	if (tail_src < b->size)
		memcpy(new_data + tail_dst, b->ptr + tail_src, b->size - tail_src);

	SAFE_FREE(b->ptr);
	b->ptr = new_data;
	b->size = new_blob_size;

	return SUCCESS;
}

i32 ensure_tag(blob *b, const int letter, const int needed_size)
{
	assert(b);
	assert(needed_size != 0);

	i32 pos = vars_pos(*b, (char)letter);
	if (pos < 0)
	{
		/* create */
		if (var_add(b, (char)letter, (u8)needed_size) != SUCCESS)
			return FAIL;
		return vars_pos(*b, (char)letter);
	}

	/* exists, ensure size */
	u8 cur_size = VAR_SIZE(b->ptr, (u32)pos);
	if (cur_size < (u8)needed_size)
	{
		if (var_resize(b, (char)letter, (u8)needed_size) != SUCCESS)
			return FAIL;
		return vars_pos(*b, (char)letter);
	}

	return pos;
}

// set

u8 var_set_str(blob *b, char letter, const char *str)
{
	assert(b);
	assert(str);
	u32 len = strlen(str);
	assert(len != 0);
	assert(len <= 0xff - 1);

	i32 pos = vars_pos(*b, letter);
	if (pos < 0)
		return FAIL;

	memcpy(VAR_VALUE(b->ptr, pos), str, len + 1);

	return SUCCESS;
}

u8 var_set_raw(blob *b, char letter, blob raw)
{
	assert(b);
	assert(b->ptr);
	assert(raw.ptr);
	u32 len = b->length;
	assert(len != 0);
	assert(len <= 0xff - 1);

	i32 pos = vars_pos(*b, letter);
	if (pos < 0)
		return FAIL;

	memcpy(VAR_VALUE(b->ptr, pos), raw.ptr, len);

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

u8 var_get_raw(blob b, char letter, blob *dest)
{
	if (!dest)
		return FAIL;

	i32 pos = vars_pos(b, letter);
	if (pos < 0)
		return FAIL;

	dest->ptr = (u8 *)VAR_VALUE(b.ptr, pos);

	return SUCCESS;
}

u8 var_get_str(blob b, char letter, char **dest)
{
	if (!dest)
		return FAIL;

	i32 pos = vars_pos(b, letter);
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

#define GETTER_FAST_IMP(type)                                                                                          \
	u8 var_get_##type##_fast(blob b, char letter, const i32 *offsets, type *dest)                                      \
	{                                                                                                                  \
		assert(dest);                                                                                                  \
		i32 pos = vars_pos_fast(b, letter, offsets);                                                                   \
		if (pos < 0)                                                                                                   \
			return FAIL;                                                                                               \
		*dest = *(type *)VAR_VALUE(b.ptr, pos);                                                                        \
		return SUCCESS;                                                                                                \
	}

GETTER_FAST_IMP(u8)
GETTER_FAST_IMP(u16)
GETTER_FAST_IMP(u32)
GETTER_FAST_IMP(u64)

GETTER_FAST_IMP(i8)
GETTER_FAST_IMP(i16)
GETTER_FAST_IMP(i32)
GETTER_FAST_IMP(i64)

// utils

#define DBG_BUF_LEN 256

static bool dbg_print_var(char *ret, char *buf, u32 size, char letter, void *ptr, u32 *index)
{
	if (size == 1)
		snprintf(buf, DBG_BUF_LEN, "\ti8 %c = %d;\n", letter, *(u8 *)ptr);
	else if (size == 2)
		snprintf(buf, DBG_BUF_LEN, "\ti16 %c = %d;\n", letter, *(u16 *)ptr);
	else if (size == 4)
		snprintf(buf, DBG_BUF_LEN, "\ti32 %c = %d;\n", letter, *(u32 *)ptr);
	else if (size == 8)
		snprintf(buf, DBG_BUF_LEN, "\ti64 %c = %lld;\n", letter, *(u64 *)ptr);
	else
	{
		snprintf(buf, DBG_BUF_LEN, "\tchar %c[] = %.*s;\n\t// u8 %c[] = { ", letter, size, (char *)ptr, letter);
		strcat(ret, buf);

		for (u32 i = 0; i < size; i++)
		{
			snprintf(buf, DBG_BUF_LEN, "0x%02x%s", *(u8 *)((u8 *)ptr + i), i != size - 1 ? ", " : "");

			strcat(ret, buf);
		}
		snprintf(buf, DBG_BUF_LEN, "};\n");
		strcat(ret, buf);

		*index += (u32)size + 2;
		return true;
	}
	return false;
}

void dbg_data_layout(blob b, char *ret)
{
	u32 data_size = b.size;

	char buf[256];

	snprintf(buf, DBG_BUF_LEN, "data_size %u\n{\n", data_size);
	strcat(ret, buf);

	u32 index = 0;
	while (index + 1 < data_size)
	{
		u8 letter = b.ptr[index];
		u8 size = b.ptr[index + 1];

		/* guard against malformed blobs */
		if ((u32)index + 2 + (u32)size > data_size)
		{
			snprintf(buf, DBG_BUF_LEN, "\t<malformed entry at %u: size %u extends beyond blob>\n", index, size);
			strcat(ret, buf);
			break;
		}

		if (dbg_print_var(ret, buf, size, letter, VAR_VALUE(b.ptr, index), &index))
			continue;

		strcat(ret, buf);

		index += (u32)size + 2;
	}

	strcat(ret, "}\n");
}