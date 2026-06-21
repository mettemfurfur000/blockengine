#include "include/file_system.h"

#include "include/folder_structure.h"
#include "include/handle.h"
#include "include/level.h"
#include "include/logging.h"
#include "include/tkv.h"
#include <lua.h>
#include <string.h>

#define SAVE_MAGIC 0x4C564C
#define SAVE_VERSION 2

void write_block_grid(layer *l, stream_t *f)
{
	u64 id = 0;
	handle32 h = {};
	for (u32 y = 0; y < l->height; y++)
		for (u32 x = 0; x < l->width; x++)
		{
			if (block_get_id(l, x, y, &id) != SUCCESS)
			{
				LOG_ERROR("failed to read the block id on %d %d", x, y);
				return;
			}
			WRITE_N(&id, f, l->block_size);
			h = block_get_var_handle(l, x, y);
			WRITE_N(&h, f, sizeof(handle32));
		}
}

void read_block_grid(layer *l, stream_t *f)
{
	u64 id = 0;
	handle32 h = {};
	for (u32 y = 0; y < l->height; y++)
		for (u32 x = 0; x < l->width; x++)
		{
			stream_read((u8 *)&id, l->block_size, f);
			if (block_set_id(l, x, y, id) != SUCCESS)
			{
				LOG_WARNING("Failed to read block at %d, %d", x, y);
				block_set_id(l, x, y, 0);
			}
			stream_read((u8 *)&h, sizeof(handle32), f);
			block_set_var_handle(l, x, y, h);
		}
}

void write_handle_table(var_handle_table *pool, stream_t *f)
{
	if (!pool->table)
	{
		u32 zero32 = 0;
		WRITE(zero32, f);
		return;
	}
	u16 cap = handle_table_capacity(pool->table);
	u32 cap32 = (u32)cap;
	WRITE(cap32, f);
	for (u16 i = 0; i < cap; ++i)
	{
		void *p = handle_table_slot_ptr(pool->table, i);
		u8 active = handle_table_slot_active(pool->table, i);
		WRITE(active, f);
		u16 generation = handle_table_slot_generation(pool->table, i);
		WRITE(generation, f);
		if (p)
			blob_vars_write(*(blob *)p, f);
		else
		{
			u32 zero32 = 0;
			WRITE(zero32, f);
		}
	}
}

void read_handle_table(var_handle_table *pool, stream_t *f)
{
	u32 slot_count = 0;
	READ(slot_count, f);
	if (slot_count == 0)
	{
		pool->table = NULL;
		return;
	}
	pool->table = handle_table_create((u16)slot_count);
	for (u16 i = 0; i < (u16)slot_count; ++i)
	{
		u8 active = 0;
		READ(active, f);
		u16 generation = 0;
		READ(generation, f);
		blob b = blob_vars_read(f);
		if (b.size == 0 || b.ptr == NULL)
			handle_table_set_slot(pool->table, i, NULL, 0, 0);
		else
		{
			blob *nb = calloc(1, sizeof(blob));
			assert(nb != NULL);
			nb->ptr = b.ptr;
			nb->size = b.size;
			handle_table_set_slot(pool->table, i, nb, generation, active);
		}
	}
}

static tkv_object build_layer_meta(layer *l, arena *tkv_arena)
{
	tkv_object obj = tkv_object_create_empty(tkv_arena);
	obj = tkv_object_add_field(obj, "uuid", TKV_VALUE_U64, TKV_STATE_CONST, &l->uuid, tkv_arena);
	obj = tkv_object_add_field(obj, "block_size", TKV_VALUE_U8, TKV_STATE_CONST, &l->block_size, tkv_arena);
	obj = tkv_object_add_field(obj, "flags", TKV_VALUE_U8, TKV_STATE_CONST, &l->flags, tkv_arena);
	obj = tkv_object_add_field(obj, "width", TKV_VALUE_U16, TKV_STATE_CONST, &l->width, tkv_arena);
	obj = tkv_object_add_field(obj, "height", TKV_VALUE_U16, TKV_STATE_CONST, &l->height, tkv_arena);
	const char *reg_name = l->registry ? (const char *)l->registry->name : "no_registry";
	obj = tkv_object_add_field(obj, "registry", TKV_VALUE_STR, TKV_STATE_CONST, reg_name, tkv_arena);
	return obj;
}

static void read_layer_meta(tkv_object obj, layer *l, room *parent)
{
	tkv_value v;
	v = tkv_get_value(obj, "uuid");
	if (v.meta.whole != UINT_MAX) l->uuid = tkv_value_to_u64(v);
	v = tkv_get_value(obj, "block_size");
	if (v.meta.whole != UINT_MAX) l->block_size = tkv_value_to_u8(v);
	l->total_bytes_per_block = l->block_size + sizeof(handle32);
	v = tkv_get_value(obj, "flags");
	if (v.meta.whole != UINT_MAX) l->flags = tkv_value_to_u8(v);
	v = tkv_get_value(obj, "width");
	if (v.meta.whole != UINT_MAX) l->width = tkv_value_to_u16(v);
	v = tkv_get_value(obj, "height");
	if (v.meta.whole != UINT_MAX) l->height = tkv_value_to_u16(v);
	v = tkv_get_value(obj, "registry");
	if (v.meta.whole != UINT_MAX)
	{
		const char *reg_name = tkv_value_to_str(v);
		if (strcmp(reg_name, "no_registry") != 0)
		{
			l->registry = find_registry(((level *)parent->parent_level)->registries, (char *)reg_name);
			if (!l->registry)
				LOG_WARNING("Registry %s not found", reg_name);
		}
	}
}

u8 save_level(level lvl)
{
	char path[256] = {};
	sprintf(path, FOLDER_LVL SEPARATOR_STR "%s.lvl", lvl.name);

	stream_t s;
	if (stream_open_write(path, COMPRESS_LEVEL, &s) != 0)
		return FAIL;

	u32 magic = SAVE_MAGIC;
	u32 version = SAVE_VERSION;
	WRITE(magic, &s);
	WRITE(version, &s);

	arena *scratch = arena_create(4096);
	arena *tkv_arena = arena_create(4096);

	tkv_object meta = tkv_object_create_empty(tkv_arena);
	meta = tkv_object_add_field(meta, "uuid", TKV_VALUE_U64, TKV_STATE_CONST, &lvl.uuid, tkv_arena);
	meta = tkv_object_add_field(meta, "name", TKV_VALUE_STR, TKV_STATE_CONST, lvl.name, tkv_arena);
	meta = tkv_object_add_field(meta, "flags", TKV_VALUE_U8, TKV_STATE_CONST, &lvl.flags, tkv_arena);
	tkv_write_to_stream(meta, &s);

	WRITE(lvl.registries.length, &s);
	for (u32 i = 0; i < lvl.registries.length; i++)
	{
		const char *reg_name = ((block_registry *)lvl.registries.data[i])->name;
		assert(reg_name);
		blob_write(blobify((char *)reg_name), &s);
	}

	WRITE(lvl.rooms.length, &s);
	for (u32 i = 0; i < lvl.rooms.length; i++)
	{
		room *r = (room *)lvl.rooms.data[i];

		tkv_arena->length = 0;
		tkv_object room_meta = tkv_object_create_empty(tkv_arena);
		room_meta = tkv_object_add_field(room_meta, "uuid", TKV_VALUE_U64, TKV_STATE_CONST, &r->uuid, tkv_arena);
		room_meta = tkv_object_add_field(room_meta, "name", TKV_VALUE_STR, TKV_STATE_CONST, r->name, tkv_arena);
		room_meta = tkv_object_add_field(room_meta, "width", TKV_VALUE_U16, TKV_STATE_CONST, &r->width, tkv_arena);
		room_meta = tkv_object_add_field(room_meta, "height", TKV_VALUE_U16, TKV_STATE_CONST, &r->height, tkv_arena);
		room_meta = tkv_object_add_field(room_meta, "flags", TKV_VALUE_U8, TKV_STATE_CONST, &r->flags, tkv_arena);
		tkv_write_to_stream(room_meta, &s);

		WRITE(r->layers.length, &s);
		for (u32 j = 0; j < r->layers.length; j++)
		{
			layer *l = (layer *)r->layers.data[j];

			tkv_arena->length = 0;
			tkv_object layer_meta = build_layer_meta(l, tkv_arena);
			tkv_write_to_stream(layer_meta, &s);

			write_block_grid(l, &s);
			write_handle_table(&l->var_pool, &s);
		}
	}

	arena_destroy(scratch);
	arena_destroy(tkv_arena);
	stream_close(&s);
	return SUCCESS;
}

u8 load_level(level *lvl, const char *name_in)
{
	char path[256] = {};
	sprintf(path, FOLDER_LVL SEPARATOR_STR "%s.lvl", name_in);

	stream_t s;
	if (stream_open_read(path, COMPRESS_LEVEL, &s) != 0)
		return FAIL;

	u32 magic, version;
	READ(magic, &s);
	READ(version, &s);
	if (magic != SAVE_MAGIC || version < 2)
	{
		LOG_ERROR("Unsupported save format: magic=0x%X version=%u", magic, version);
		stream_close(&s);
		return FAIL;
	}

	arena *scratch = arena_create(4096);
	arena *tkv_arena = arena_create(65536);

	{
		tkv_object meta = tkv_read_from_stream(&s, scratch, tkv_arena);
		if (!meta)
		{
			LOG_ERROR("Failed to read level metadata");
			arena_destroy(scratch);
			arena_destroy(tkv_arena);
			stream_close(&s);
			return FAIL;
		}
		tkv_value v;
		v = tkv_get_value(meta, "uuid");
		if (v.meta.whole != UINT_MAX) lvl->uuid = tkv_value_to_u64(v);
		v = tkv_get_value(meta, "name");
		if (v.meta.whole != UINT_MAX) lvl->name = strdup(tkv_value_to_str(v));
		v = tkv_get_value(meta, "flags");
		if (v.meta.whole != UINT_MAX) lvl->flags = tkv_value_to_u8(v);
	}

	u32 reg_count;
	READ(reg_count, &s);
	for (u32 i = 0; i < reg_count; i++)
	{
		char *name = blob_read(&s).str;
		block_registry *reg = find_registry(lvl->registries, name);
		if (!reg)
		{
			LOG_WARNING("Registry %s not found in existing level registries, loading from disk", name);
			reg = registry_load(name);
			if (!reg)
			{
				LOG_WARNING("Failed to load registry %s", name);
				free(name);
				continue;
			}
		}
		(void)vec_push(&lvl->registries, reg);
	}

	u32 room_count;
	READ(room_count, &s);
	for (u32 i = 0; i < room_count; i++)
	{
		tkv_object room_meta = tkv_read_from_stream(&s, scratch, tkv_arena);
		if (!room_meta)
		{
			LOG_ERROR("Failed to read room metadata");
			break;
		}

		room *r = calloc(1, sizeof(room));
		assert(r != NULL);
		r->parent_level = lvl;

		tkv_value v;
		v = tkv_get_value(room_meta, "uuid");
		if (v.meta.whole != UINT_MAX) r->uuid = tkv_value_to_u64(v);
		v = tkv_get_value(room_meta, "width");
		if (v.meta.whole != UINT_MAX) r->width = tkv_value_to_u16(v);
		v = tkv_get_value(room_meta, "height");
		if (v.meta.whole != UINT_MAX) r->height = tkv_value_to_u16(v);
		v = tkv_get_value(room_meta, "name");
		if (v.meta.whole != UINT_MAX) r->name = strdup(tkv_value_to_str(v));
		v = tkv_get_value(room_meta, "flags");
		if (v.meta.whole != UINT_MAX) r->flags = tkv_value_to_u8(v);

		u32 layer_count;
		READ(layer_count, &s);
		vec_reserve(&r->layers, layer_count);

		for (u32 j = 0; j < layer_count; j++)
		{
			tkv_object layer_meta = tkv_read_from_stream(&s, scratch, tkv_arena);
			if (!layer_meta)
			{
				LOG_ERROR("Failed to read layer metadata");
				break;
			}

			layer *l = calloc(1, sizeof(layer));
			assert(l != NULL);

			read_layer_meta(layer_meta, l, r);
			init_layer(l, r);
			read_block_grid(l, &s);
			read_handle_table(&l->var_pool, &s);

			r->layers.data[j] = l;
		}
		r->layers.length = layer_count;

		room_create_world(r, NULL);
		(void)vec_push(&lvl->rooms, r);
	}

	arena_destroy(scratch);
	arena_destroy(tkv_arena);
	stream_close(&s);
	return SUCCESS;
}

u8 load_level_ack_registry(level *lvl, const char *name_in, block_registry *ack_reg)
{
	char path[256] = {};
	sprintf(path, FOLDER_LVL SEPARATOR_STR "%s.lvl", name_in);

	stream_t s;
	if (stream_open_read(path, COMPRESS_LEVEL, &s) != 0)
		return FAIL;

	u32 magic, version;
	READ(magic, &s);
	READ(version, &s);
	if (magic != SAVE_MAGIC || version < 2)
	{
		LOG_ERROR("Unsupported save format: magic=0x%X version=%u", magic, version);
		stream_close(&s);
		return FAIL;
	}

	arena *scratch = arena_create(4096);
	arena *tkv_arena = arena_create(65536);

	{
		tkv_object meta = tkv_read_from_stream(&s, scratch, tkv_arena);
		if (!meta)
		{
			LOG_ERROR("Failed to read level metadata");
			arena_destroy(scratch);
			arena_destroy(tkv_arena);
			stream_close(&s);
			return FAIL;
		}
		tkv_value v;
		v = tkv_get_value(meta, "uuid");
		if (v.meta.whole != UINT_MAX) lvl->uuid = tkv_value_to_u64(v);
		v = tkv_get_value(meta, "name");
		if (v.meta.whole != UINT_MAX) lvl->name = strdup(tkv_value_to_str(v));
		v = tkv_get_value(meta, "flags");
		if (v.meta.whole != UINT_MAX) lvl->flags = tkv_value_to_u8(v);
	}

	u32 reg_count;
	READ(reg_count, &s);
	for (u32 i = 0; i < reg_count; i++)
	{
		char *name = blob_read(&s).str;
		if (strcmp(name, ack_reg->name) == 0)
		{
			block_registry *reg = ack_reg;
			(void)vec_push(&lvl->registries, reg);
		}
		else
		{
			LOG_ERROR("Registry %s does not match acknowledged registry %s", name, ack_reg->name);
			abort();
		}
	}

	u32 room_count;
	READ(room_count, &s);
	for (u32 i = 0; i < room_count; i++)
	{
		tkv_object room_meta = tkv_read_from_stream(&s, scratch, tkv_arena);
		if (!room_meta)
		{
			LOG_ERROR("Failed to read room metadata");
			break;
		}

		room *r = calloc(1, sizeof(room));
		assert(r != NULL);
		r->parent_level = lvl;

		tkv_value v;
		v = tkv_get_value(room_meta, "uuid");
		if (v.meta.whole != UINT_MAX) r->uuid = tkv_value_to_u64(v);
		v = tkv_get_value(room_meta, "width");
		if (v.meta.whole != UINT_MAX) r->width = tkv_value_to_u16(v);
		v = tkv_get_value(room_meta, "height");
		if (v.meta.whole != UINT_MAX) r->height = tkv_value_to_u16(v);
		v = tkv_get_value(room_meta, "name");
		if (v.meta.whole != UINT_MAX) r->name = strdup(tkv_value_to_str(v));
		v = tkv_get_value(room_meta, "flags");
		if (v.meta.whole != UINT_MAX) r->flags = tkv_value_to_u8(v);

		u32 layer_count;
		READ(layer_count, &s);
		vec_reserve(&r->layers, layer_count);

		for (u32 j = 0; j < layer_count; j++)
		{
			tkv_object layer_meta = tkv_read_from_stream(&s, scratch, tkv_arena);
			if (!layer_meta)
			{
				LOG_ERROR("Failed to read layer metadata");
				break;
			}

			layer *l = calloc(1, sizeof(layer));
			assert(l != NULL);

			read_layer_meta(layer_meta, l, r);
			init_layer(l, r);
			read_block_grid(l, &s);
			read_handle_table(&l->var_pool, &s);

			r->layers.data[j] = l;
		}
		r->layers.length = layer_count;

		room_create_world(r, NULL);
		(void)vec_push(&lvl->rooms, r);
	}

	arena_destroy(scratch);
	arena_destroy(tkv_arena);
	stream_close(&s);
	return SUCCESS;
}
