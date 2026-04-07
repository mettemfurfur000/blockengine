#include "include/update_system.h"
#include "include/data_io.h"
#include "include/general.h"

#include "vec/src/vec.h"

update_accumulator update_acc_new()
{
	update_accumulator ret = {};
	ret.update_stream.mode = STREAM_BUF;
	return ret;
}

void update_acc_free(update_accumulator *a)
{
	vec_deinit(&a->update_stream.handle.raw.bytes);
}

static inline void write_variable(stream dest, u8 width, u64 val)
{
	assert(width % 2 == 0 || width == 1);

	switch (width)
	{
	case sizeof(u8):;
		u8 t8 = val;
		WRITE(t8, dest);
		break;
	case sizeof(u16):;
		u16 t16 = val;
		WRITE(t16, dest);
		break;
	case sizeof(u32):;
		u32 t32 = val;
		WRITE(t32, dest);
		break;
	case sizeof(u64):;
		WRITE(val, dest);
		break;
	default:
		assert(0 && "unreachable!");
	}
}

static inline void read_variable(stream src, u8 width, u64 *dest)
{
	assert(width % 2 == 0 || width == 1);

	switch (width)
	{
	case sizeof(u8):;
		u8 t8 = 0;
		READ(t8, src);
		*dest = t8;
		break;
	case sizeof(u16):;
		u16 t16 = 0;
		READ(t16, src);
		*dest = t16;
		break;
	case sizeof(u32):;
		u32 t32 = 0;
		READ(t32, src);
		*dest = t32;
		break;
	case sizeof(u64):;
		u64 t64 = 0;
		READ(t64, src);
		*dest = t64;
		break;
	default:
		assert(0 && "unreachable!");
	}
}

// blocks logic
//

void update_block_push(update_accumulator *a, u16 x, u16 y, u64 id, u8 true_width)
{
	WRITE(x, &a->update_stream);
	WRITE(y, &a->update_stream);
	write_variable(&a->update_stream, true_width, id);

	a->update_count++;
}

update_block update_block_read(update_accumulator a, u8 true_width)
{
	assert(true_width % 2 == 0 || true_width == 1);

	// const u8 update_width = 4 + true_width;
	update_block update = {};

	READ(update.x, &a.update_stream);
	READ(update.y, &a.update_stream);
	read_variable(&a.update_stream, true_width, &update.id);

	return update;
}

// var logic
//

void update_var_push(update_accumulator *a, u16 x, u16 y, handle32 handle)
{
	WRITE(x, &a->update_stream);
	WRITE(y, &a->update_stream);
	WRITE(handle, &a->update_stream);
	a->update_count++;
}

update_varhandle update_var_read(update_accumulator a)
{
	update_varhandle update = {};

	READ(update.x, &a.update_stream);
	READ(update.y, &a.update_stream);
	READ(update.h, &a.update_stream);

	return update;
}

// component logic
//

void update_component_new_push(update_accumulator *a, handle32 handle, blob b)
{
	u8 type = COMPONENT_UPDATE_NEW;
	WRITE(type, &a->update_stream);
	WRITE(handle, &a->update_stream);

	blob_vars_write(b, &a->update_stream);

	a->update_count++;
}

#define UPDATE_COMP_SET_GEN(T)                                                                                         \
	void update_component_set_push_##T(update_accumulator *a, handle32 handle, char c, T value)                        \
	{                                                                                                                  \
		u8 type = COMPONENT_UPDATE_SET;                                                                                \
		WRITE(type, &a->update_stream);                                                                                \
		WRITE(handle, &a->update_stream);                                                                              \
		WRITE(c, &a->update_stream);                                                                                   \
		u8 len = sizeof(T);                                                                                            \
		WRITE(len, &a->update_stream);                                                                                 \
		WRITE(value, &a->update_stream);                                                                               \
	}

void update_component_set_push(update_accumulator *a, handle32 handle, char c, u8 len, u8 *ptr)
{
	u8 type = COMPONENT_UPDATE_SET;
	WRITE(type, &a->update_stream);
	WRITE(handle, &a->update_stream);

	WRITE(c, &a->update_stream);
	WRITE(len, &a->update_stream);
	WRITE_N(ptr, &a->update_stream, len);
}

UPDATE_COMP_SET_GEN(u8)
UPDATE_COMP_SET_GEN(u16)
UPDATE_COMP_SET_GEN(u32)
UPDATE_COMP_SET_GEN(u64)

UPDATE_COMP_SET_GEN(i8)
UPDATE_COMP_SET_GEN(i16)
UPDATE_COMP_SET_GEN(i32)
UPDATE_COMP_SET_GEN(i64)

void update_component_add_push(update_accumulator *a, handle32 handle, char c, u8 size)
{
	u8 type = COMPONENT_UPDATE_ADD;
	WRITE(type, &a->update_stream);
	WRITE(handle, &a->update_stream);

	WRITE(c, &a->update_stream);
	WRITE(size, &a->update_stream);
}

void update_component_delete_push(update_accumulator *a, handle32 handle, char c)
{
	u8 type = COMPONENT_UPDATE_DELETE;
	WRITE(type, &a->update_stream);
	WRITE(handle, &a->update_stream);

	WRITE(c, &a->update_stream);
}

void update_component_resize_push(update_accumulator *a, handle32 handle, char c, u8 new_size)
{
	u8 type = COMPONENT_UPDATE_RESIZE;
	WRITE(type, &a->update_stream);
	WRITE(handle, &a->update_stream);

	WRITE(c, &a->update_stream);
	WRITE(new_size, &a->update_stream);
}

void update_component_rename_push(update_accumulator *a, handle32 handle, char c, char new_name)
{
	u8 type = COMPONENT_UPDATE_RENAME;
	WRITE(type, &a->update_stream);
	WRITE(handle, &a->update_stream);

	WRITE(c, &a->update_stream);
	WRITE(new_name, &a->update_stream);
}

static u8 var_set_buf[256] = {};

update_var_component update_component_read(update_accumulator a)
{
	update_var_component u = {};

	READ(u.type, &a.update_stream);

	switch (u.type)
	{
	case COMPONENT_UPDATE_NEW:
		READ(u.h, &a.update_stream);
		blob *nb = calloc(1, sizeof(blob));
		assert(nb != NULL);
		*nb = blob_vars_read(&a.update_stream);
		u.blob = nb;
		break;
	case COMPONENT_UPDATE_SET:
		READ(u.h, &a.update_stream);
		READ(u.letter, &a.update_stream);
		READ(u.size, &a.update_stream);
		READ_N(var_set_buf, &a.update_stream, u.size);
		u.raw = var_set_buf;
		break;
	case COMPONENT_UPDATE_ADD:
		READ(u.h, &a.update_stream);
		READ(u.letter, &a.update_stream);
		READ(u.size, &a.update_stream);
		break;
	case COMPONENT_UPDATE_DELETE:
		READ(u.h, &a.update_stream);
		READ(u.letter, &a.update_stream);
		break;
	case COMPONENT_UPDATE_RESIZE:
		READ(u.h, &a.update_stream);
		READ(u.letter, &a.update_stream);
		READ(u.size, &a.update_stream);
		break;
	case COMPONENT_UPDATE_RENAME:
		READ(u.h, &a.update_stream);
		READ(u.letter, &a.update_stream);
		READ(u.new_char, &a.update_stream);
		break;
	}

	return u;
}
