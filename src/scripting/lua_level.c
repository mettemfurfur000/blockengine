#include "include/scripting/level.h"

#include "include/scripting_bindings.h"

#include "include/block_registry.h"
#include "include/events.h"
#include "include/file_system.h"
#include "include/flags.h"
#include "include/handle.h"
#include "include/level.h"
#include "include/logging.h"
#include "include/scripting.h"
#include "include/scripting_var_handles.h"

#include "include/config.h"

#include <lauxlib.h>
#include <lua.h>

#include "include/block_entity.h"

static int lua_level_create(lua_State *L)
{
	NEW_USER_OBJECT(L, Level, level_create(luaL_checkstring(L, 1)));
	return 1;
}

static int lua_load_level(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	LUA_CHECK_USER_OBJECT(L, BlockRegistry, RegWrapper, 2)
	level *out = calloc(1, sizeof(level));

	if (load_level_ack_registry(out, name, RegWrapper->reg) == SUCCESS)
	{
		NEW_USER_OBJECT(L, Level, out);
	}
	else
		lua_pushnil(L);

	return 1;
}

static int lua_save_level(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);

	lua_pushboolean(L, save_level(*wrapper->lvl) == SUCCESS);

	return 1;
}

static int lua_level_apply_updates(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);

	for (u32 ri = 0; ri < wrapper->lvl->rooms.length; ri++)
	{
		room *r = wrapper->lvl->rooms.data[ri];

		for (u32 i = 0; i < r->layers.length; i++)
		{
			block_apply_updates(r->layers.data[i]);
		}
	}

	return 0;
}

static int lua_level_registry_serialize(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	const u32 index = luaL_checkinteger(L, 2);

	if (index >= wrapper->lvl->registries.length)
		luaL_error(L, "Index out of range");

	lua_pushboolean(L, registry_save(wrapper->lvl->registries.data[index]) == SUCCESS ? 1 : 0);
	return 1;
}

static int lua_level_registry_deserialize(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	const char *name = luaL_checkstring(L, 2);

	block_registry *r = registry_load(name);

	if (r)
	{
		(void)vec_push(&wrapper->lvl->registries, r);
		NEW_USER_OBJECT(L, BlockRegistry, r);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int lua_level_load_registry(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	const char *registry_name = luaL_checkstring(L, 2);

	block_registry *r = registry_load(registry_name);

	if (r != NULL)
	{
		(void)vec_push(&wrapper->lvl->registries, r);
		NEW_USER_OBJECT(L, BlockRegistry, r);
	}
	else
	{
		free(r);
		lua_pushnil(L);
	}

	return 1;
}

static int lua_level_get_registries(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);

	lua_newtable(L);

	vec_void_t *registries = &wrapper->lvl->registries;

	for (u32 i = 0; i < registries->length; i++)
	{
		NEW_USER_OBJECT(L, BlockRegistry, registries->data[i]);
		lua_seti(L, -2, i + 1);
	}

	return 1;
}

static int lua_level_add_existing(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	LUA_CHECK_USER_OBJECT(L, BlockRegistry, reg_wrapper, 2);

	(void)vec_push(&wrapper->lvl->registries, reg_wrapper->reg);

	return 0;
}

static int lua_level_gc(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	free_level(wrapper->lvl);

	free(wrapper->lvl);

	return 0;
}

static int lua_level_get_name(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	lua_pushstring(L, wrapper->lvl->name);
	return 1;
}

static int lua_level_get_room(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	u32 index = luaL_checkinteger(L, 2);

	if (index >= wrapper->lvl->rooms.length)
		luaL_error(L, "Index out of range");

	room *r = wrapper->lvl->rooms.data[index];

	NEW_USER_OBJECT(L, Room, r);

	return 1;
}

static int lua_level_get_room_count(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	lua_pushinteger(L, wrapper->lvl->rooms.length);
	return 1;
}

static int lua_level_get_room_by_name(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	const char *name = luaL_checkstring(L, 2);

	for (u32 i = 0; i < wrapper->lvl->rooms.length; i++)
	{
		if (strcmp(((room *)wrapper->lvl->rooms.data[i])->name, name) == 0)
		{
			NEW_USER_OBJECT(L, Room, wrapper->lvl->rooms.data[i]);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

static int lua_level_new_room(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	const char *name = luaL_checkstring(L, 2);
	int w = luaL_checkinteger(L, 3);
	int h = luaL_checkinteger(L, 4);

	NEW_USER_OBJECT(L, Room, room_create(wrapper->lvl, name, w, h));
	return 1;
}

static int lua_room_get_name(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
	lua_pushstring(L, wrapper->r->name);
	return 1;
}

static int lua_room_get_size(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
	lua_pushinteger(L, wrapper->r->width);
	lua_pushinteger(L, wrapper->r->height);
	return 2;
}

static int lua_room_new_layer(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
	const char *registry_name = luaL_checkstring(L, 2);

	int bytes_per_block = luaL_checkinteger(L, 3);
	int flags = luaL_checkinteger(L, 4);

	block_registry *reg = find_registry((((level *)wrapper->r->parent_level)->registries), (char *)registry_name);

	if (!reg)
		luaL_error(L, "Registry %s not found", registry_name);

	NEW_USER_OBJECT(L, Layer, layer_create(wrapper->r, reg, bytes_per_block, flags));

	return 1;
}

static int lua_room_get_layer(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
	u32 index = luaL_checkinteger(L, 2);

	if (index >= wrapper->r->layers.length)
		luaL_error(L, "Index out of range");

	NEW_USER_OBJECT(L, Layer, wrapper->r->layers.data[index]);
	return 1;
}

static int lua_room_get_layer_count(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
	lua_pushinteger(L, wrapper->r->layers.length);
	return 1;
}

// layer-related functions

// pastes the block from the registry into the layer
// also triggers a block create event

static int lua_layer_paste_block(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	u64 id = luaL_checknumber(L, 4);

	if (x >= wrapper->l->width || y >= wrapper->l->height)
		luaL_error(L, "Coordinates out of range - %d,%d for layer size %d,%d", x, y, wrapper->l->width,
				   wrapper->l->height);

	if (!wrapper->l->registry)
		luaL_error(L, "Layer has no registry");

	if (id >= wrapper->l->registry->resources.length)
		luaL_error(L, "Block ID out of range - %d out of total %d blocks", id, wrapper->l->registry->resources.length);

	block_resources *res = &wrapper->l->registry->resources.data[id];

	u64 old_id = 0;

	if (id == 0)
	{
		if (block_delete_vars(wrapper->l, x, y) != SUCCESS)
			luaL_error(L, "Failed to delete vars at: %d, %d", x, y);
	}
	else
	{
		if (layer_copy_vars(wrapper->l, x, y, id == 0 ? (blob){} : res->vars_sample) != SUCCESS)
			luaL_error(L, "Failed to copy vars at: %d, %d", x, y);
	}

	if (block_get_id(wrapper->l, x, y, &old_id) != SUCCESS)
		luaL_error(L, "Failed to get id at: %d, %d", x, y);
	if (block_set_id(wrapper->l, x, y, id) != SUCCESS)
		luaL_error(L, "Failed to set id at: %d, %d", x, y);

	/* Push block update to layer accumulator */
	// update_block_push(&wrapper->l->id_updates, x, y, id, wrapper->l->block_size);

	block_update_event e = {
		.type = ENGINE_BLOCK_CREATE,
		.x = x,
		.y = y,
		.previous_id = old_id,
		.new_id = id,
		.layer_ptr = wrapper->l,
		.room_ptr = wrapper->l->parent_room,
	};

	SDL_PushEvent((SDL_Event *)&e);
	return 0;
}

static int lua_layer_move_block(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	u32 delta_x = luaL_checknumber(L, 4);
	u32 delta_y = luaL_checknumber(L, 5);

	lua_pushboolean(L, block_move(wrapper->l, x, y, delta_x, delta_y) == SUCCESS);

	return 1;
}

static int lua_get_block_input_handler(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);

	const char *name = luaL_checkstring(L, 4);

	u64 id = 0;
	if (block_get_id(wrapper->l, x, y, &id) != SUCCESS)
	{
		lua_pushnil(L);
		return 1;
	}

	vec_int_t *refs = &wrapper->l->registry->resources.data[id].input_refs;
	vec_str_t *ref_names = &wrapper->l->registry->resources.data[id].input_names;

	for (u32 i = 0; i < refs->length; i++)
		if (strcmp(ref_names->data[i], name) == 0)
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, refs->data[i]);
			return 1;
		}

	lua_pushnil(L);
	return 1;
}

static int lua_layer_get_size(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	lua_pushinteger(L, wrapper->l->width);
	lua_pushinteger(L, wrapper->l->height);
	lua_pushinteger(L, wrapper->l->block_size);
	// lua_pushinteger(L, wrapper->sizeof(handle32));

	return 3;
}

static int lua_layer_set_static(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);
	bool val = luaL_checkinteger(L, 2);

	FLAG_SET(wrapper->l->flags, LAYER_FLAG_STATIC, val);
	return 0;
}

static int lua_layer_for_each(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u64 filter = luaL_checkinteger(L, 2); // a block id that is beink searched (?)
	luaL_checktype(L, 3, LUA_TFUNCTION);  // callback

	u32 w = wrapper->l->width;
	u32 h = wrapper->l->height;

	u64 id;

	for (u32 j = 0; j < h; j++)
		for (u32 i = 0; i < w; i++)
		{
			if (block_get_id(wrapper->l, i, j, &id) != SUCCESS)
			{
				LOG_ERROR("Error getting a block at %d %d");
				return 0;
			}

			// LOG_DEBUG("foreach found %d agains filter id %d", id, filter);

			if (id == filter)
			{
				lua_pushvalue(L, 3);
				lua_pushinteger(L, i);
				lua_pushinteger(L, j);

				if (lua_pcall(g_L, 2, 0, 0) != 0)
				{
					LOG_ERROR("Error calling a callback: %s", lua_tostring(g_L, -1));
					lua_pop(g_L, 1);
					return 0;
				}
			}
		}

	return 0;
}

static int lua_layer_set_id(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	u64 id = luaL_checknumber(L, 4);

	if (block_set_id(wrapper->l, x, y, id) == SUCCESS)
	{
		/* Push block update to layer accumulator */
		// push_block_update(&wrapper->l->id_updates, x, y, id, wrapper->l->block_size);
		lua_pushboolean(L, 1);
	}
	else
	{
		lua_pushboolean(L, 0);
	}

	return 1;
}

static int lua_block_get_id(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	u64 id = 0;

	if (block_get_id(wrapper->l, x, y, &id) == SUCCESS)
		lua_pushnumber(L, id);
	else
		lua_pushnil(L);

	return 1;
}

static int lua_block_get_vars(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	/* Return a VarHandle instead of raw Vars pointer. First return boolean success, then handle or nil */
	handle32 h = block_get_var_handle(wrapper->l, x, y);

	if (!handle_is_valid(wrapper->l->var_pool.table, h)) // not valid nuh uh
		lua_pushnil(L);
	else
		push_varhandle(L, wrapper->l, h);

	return 1;
}

static int lua_block_copy_vars(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	/* Accept either a Vars userdata (blob pointer) or a VarHandle userdata */
	blob *src_blob = NULL;

	// if (luaL_testudata(L, 4, "Vars"))
	// {
	//     LuaHolder *wrapper_vars = (LuaHolder *)luaL_checkudata(L, 4, "Vars");
	//     src_blob = wrapper_vars->b;
	// }
	// else
	if (luaL_testudata(L, 4, "VarHandle"))
	{
		src_blob = get_blob_from_varhandle(L, 4);
		if (!src_blob)
		{
			lua_pushboolean(L, 0);
			return 1;
		}
	}
	else
	{
		luaL_error(L, "Expected VarHandle as 4th argument");
	}

	lua_pushboolean(L, layer_copy_vars(wrapper->l, x, y, *src_blob) == SUCCESS);
	return 1;
}

static int lua_bprintf(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);
	u64 character_id = luaL_checkinteger(L, 2);
	u32 orig_x = luaL_checkinteger(L, 3);
	u32 orig_y = luaL_checkinteger(L, 4);
	u32 limit = luaL_checkinteger(L, 5);
	const char *format = luaL_checkstring(L, 6);
	bprintf(wrapper->l, character_id, orig_x, orig_y, limit, format);
	return 0;
}

typedef struct
{
	layer *l;
	f32 dt;
	lua_State *L;
	u64 value;
} tick_iter_data;

u32 block_entity_collision_script(lua_State *L, block_entity *e, collision_result col_res)
{
	u64 id = e->block_id;
	layer *l = e->parent_layer;

	assert(l);

	if (id == 0 || id >= l->registry->resources.length)
		return SUCCESS;

	i32 ref = l->registry->resources.data[id].entity_collision_ref;
	if (ref == 0)
		return SUCCESS;

	assert(lua_rawgeti(L, LUA_REGISTRYINDEX, ref) == LUA_TFUNCTION);

	lua_pushvalue(L, 1);
	NEW_USER_OBJECT_HANDLE32(L, BlockEntity, l, e->handle);
	lua_pushinteger(L, col_res.block_pos.x);
	lua_pushinteger(L, col_res.block_pos.y);
	lua_pushinteger(L, col_res.block_id);
	lua_pushnumber(L, col_res.hit_pos.x);
	lua_pushnumber(L, col_res.hit_pos.y);

	if (lua_pcall(L, 7, 0, 0) != 0)
	{
		LOG_ERROR("Error calling an entity collision callback : %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_pushnil(L);
		return FAIL;
	}

	return SUCCESS;
}

u32 block_entity_tick_script(lua_State *L, layer *l, block_entity *e, u64 value)
{
	u64 id = e->block_id;

	if (id == 0 || id >= l->registry->resources.length)
		return SUCCESS; // air and invalid blocks dont need anything

	i32 ref = l->registry->resources.data[id].entity_tick_ref;
	if (ref == 0)
		return SUCCESS; // no ref means block has no tick logic

	assert(lua_rawgeti(L, LUA_REGISTRYINDEX, ref) == LUA_TFUNCTION); // ref doesnt point to a function...

	lua_pushvalue(L, 1); // still pushes that layer value to the ticker function
	NEW_USER_OBJECT_HANDLE32(L, BlockEntity, l, e->handle);
	lua_pushinteger(L, value);

	if (lua_pcall(L, 3, 0, 0) != 0)
	{
		LOG_ERROR("Error calling an entity tick callback: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_pushnil(L);
		return FAIL;
	}

	return SUCCESS;
}

u32 block_entity_foreach(handle32 h, void *ptr, void *user_data)
{
	block_entity *e = (block_entity *)ptr;
	tick_iter_data *info = user_data;
	assert(e);

	layer *l = e->parent_layer;
	assert(l);

	block_entity_update(e, info->dt);

	if (!handle_is_valid(info->l->block_entity_pool, h))
		return SUCCESS;

	collision_result res = {};

	if (l->registry->resources.data[e->block_id].entity_collision_ref)
		if (block_entity_check_collision_swept(e, l, info->dt, &res))
			if (block_entity_collision_script(info->L, e, res) != SUCCESS)
				return FAIL;

	if (block_entity_tick_script(info->L, info->l, e, info->value) != SUCCESS)
		return FAIL;

	if (!handle_is_valid(info->l->block_entity_pool, h))
		return SUCCESS;

	return SUCCESS;
}

u32 block_entity_tick_each(handle32 h, void *ptr, void *user_data)
{
	block_entity *e = (block_entity *)ptr;
	tick_iter_data *info = user_data;
	assert(e);

	if (block_entity_tick_script(info->L, info->l, e, info->value) != SUCCESS)
		return FAIL;

	// check if its still valid

	if (!handle_is_valid(info->l->block_entity_pool, h)) // the entity has destroyed itself...
		return SUCCESS;

	block_entity_update(e, info->dt);

	return SUCCESS;
}

void layer_tick_entities(lua_State *L, layer *l, float dt, u64 value)
{
	tick_iter_data info = {.dt = dt, .l = l, .L = L, .value = value};

	l->block_entity_count_estimate = handle_table_iterate(l->block_entity_pool, block_entity_foreach, &info);
}

static int lua_layer_tick_blocks(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);
	const u64 value = luaL_checkinteger(L, 2);

	const u32 w = wrapper->l->width;
	const u32 h = wrapper->l->height;

	u64 id;

	int ref = 0;

	const block_registry *reg = wrapper->l->registry;

	for (u32 j = 0; j < h; j++)
		for (u32 i = 0; i < w; i++)
		{
			if (block_get_id(wrapper->l, i, j, &id) != SUCCESS)
			{
				LOG_ERROR("tick errored for block %d:%d on layer %lld", i, j, wrapper->l->uuid);
				lua_pushnil(L);
				return 1;
			}

			if (id == 0 || id >= wrapper->l->registry->resources.length) // ignore invalid values or empty blocks
				continue;

			ref = reg->resources.data[id].input_tick_ref; // get the tick function from the registry
			if (ref == 0)
				continue;

			if (lua_rawgeti(L, LUA_REGISTRYINDEX, ref) == LUA_TFUNCTION) // execute it
			{
				lua_pushvalue(L, 1);
				lua_pushinteger(L, i);
				lua_pushinteger(L, j);
				lua_pushinteger(L, value);

				if (lua_pcall(L, 4, 0, 0) != 0)
				{
					LOG_ERROR("Error calling a tick callback: %s", lua_tostring(L, -1));
					lua_pop(L, 1);
					lua_pushnil(L);
					return 1;
				}
			}
		}

	if (wrapper->l->block_entity_pool)
		layer_tick_entities(L, wrapper->l, 1.0f / TPS, value);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_layer_new_entity(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u64 block_id = (u64)luaL_checkinteger(L, 2);
	float x = (float)luaL_checknumber(L, 3);
	float y = (float)luaL_checknumber(L, 4);

	handle32 h = layer_add_block_entity(wrapper->l, block_id, x, y);
	if (!handle_is_valid(wrapper->l->block_entity_pool, h))
	{
		luaL_error(L, "Failed to create block entity on layer %lld", wrapper->l->uuid);
		lua_pushnil(L);
		return 1;
	}

	NEW_USER_OBJECT_HANDLE32(L, BlockEntity, wrapper->l, h);
	return 1;
}

u32 iter_entities_get(handle32 h, void *ptr, void *user_data)
{
	void **inputs = (void **)user_data;
	lua_State *L = (lua_State *)inputs[0];
	layer *l = (layer *)inputs[1];
	block_entity *e = (block_entity *)ptr;

	assert(e);

	handle32convertor c = {.h = h};
	lua_pushinteger(L, c.i);
	NEW_USER_OBJECT_HANDLE32(L, BlockEntity, l, h);
	lua_settable(L, -3);
	return SUCCESS;
}

static int lua_layer_get_entities(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	if (!wrapper->l->block_entity_pool)
	{
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);

	void *pointers[] = {(void *)L, (void *)wrapper->l};

	handle_table_iterate(wrapper->l->block_entity_pool, iter_entities_get, pointers);

	return 1;
}

static int lua_layer_cleanup_unused_vars(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u8 result = layer_cleanup_unused_vars(wrapper->l);

	lua_pushboolean(L, result == SUCCESS);
	return 1;
}

void lua_layer_register(lua_State *L)
{
	const static luaL_Reg layer_methods[] = {
		{		   "get_size",			 lua_layer_get_size},
		{		   "for_each",			 lua_layer_for_each},
		{			 "set_id",			   lua_layer_set_id},
		{			 "get_id",			   lua_block_get_id},
		{		 "move_block",		   lua_layer_move_block},
		{		 "paste_block",			lua_layer_paste_block},
		{	 "get_input_handler",	  lua_get_block_input_handler},
		{		 "set_static",		   lua_layer_set_static},
		{		   "get_vars",			 lua_block_get_vars},
		{		   "set_vars",			 lua_block_copy_vars},
		{			 "bprint",				   lua_bprintf},
		{			   "tick",		 lua_layer_tick_blocks},
		{			   "uuid",				 lua_uuid_shared},
		{		 "new_entity",		   lua_layer_new_entity},
		{		 "get_entities",		 lua_layer_get_entities},
		{"cleanup_unused_vars", lua_layer_cleanup_unused_vars},
		{				 NULL,						  NULL},
	};

	luaL_newmetatable(L, "Layer");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, layer_methods, 0);
}

void lua_level_register(lua_State *L)
{
	const static luaL_Reg level_methods[] = {
		{		 "load_registry",		  lua_level_load_registry},
		{	 "serialize_registry",   lua_level_registry_serialize},
		{"deserialize_registry", lua_level_registry_deserialize},
		{		 "apply_updates",		  lua_level_apply_updates},
		{		 "get_registries",	   lua_level_get_registries},
		{		 "add_existing",		 lua_level_add_existing},
		{		 "get_room_count",	   lua_level_get_room_count},
		{		   "find_room",	  lua_level_get_room_by_name},
		{			"get_name",			 lua_level_get_name},
		{			"get_room",			 lua_level_get_room},
		{			"new_room",			 lua_level_new_room},
		{				"uuid",				 lua_uuid_shared},
		{				"__gc",				   lua_level_gc},
		{				  NULL,						   NULL},
	};

	luaL_newmetatable(L, "Level");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, level_methods, 0);
}

void lua_room_register(lua_State *L)
{
	const static luaL_Reg room_methods[] = {
		{		 "get_name",		 lua_room_get_name},
		{		 "get_size",		 lua_room_get_size},
		{		 "get_layer",		  lua_room_get_layer},
		{"get_layer_count", lua_room_get_layer_count},
		{		 "new_layer",		  lua_room_new_layer},
		{		   "uuid",			 lua_uuid_shared},
		{			 NULL,					 NULL},
	};

	luaL_newmetatable(L, "Room");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, room_methods, 0);
}

void lua_level_editing_lib_register(lua_State *L)
{
	const static luaL_Reg level_editing_methods[] = {
		{"create_level", lua_level_create},
		{	 "load_level",   lua_load_level},
		{	 "save_level",   lua_save_level},
		{		  NULL,			   NULL},
	};

	luaL_newlib(L, level_editing_methods);
	lua_setglobal(L, "le");
}