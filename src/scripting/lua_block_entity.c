#include "include/scripting/entity.h"

#include "include/block_entity.h"
#include "include/handle.h"
#include "include/scripting.h"

#include "include/level.h"
#include "include/scripting_var_handles.h"
#include <lua.h>

static int lua_entity_new_index(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, BlockEntity, wrapper, 1);
	const char *property = luaL_checkstring(L, 2);

	block_entity *e = layer_get_block_entity(wrapper->l, wrapper->h);
	if (!e)
		return 0;

	else if (strcmp(property, "pos") == 0 || strcmp(property, "position") == 0)
	{
		float x = (float)luaL_checknumber(L, 3);
		float y = (float)luaL_checknumber(L, 4);
		block_entity_set_pos(e, x, y);
	}
	else if (strcmp(property, "vel") == 0 || strcmp(property, "velocity") == 0)
	{
		float vx = (float)luaL_checknumber(L, 3);
		float vy = (float)luaL_checknumber(L, 4);
		block_entity_set_vel(e, vx, vy);
	}
	else if (strcmp(property, "rotation") == 0)
	{
		float rotation = (float)luaL_checknumber(L, 3);
		block_entity_set_rotation(e, rotation);
	}
	else if (strcmp(property, "scale") == 0)
	{
		float scale_x = (float)luaL_checknumber(L, 3);
		float scale_y = (float)luaL_checknumber(L, 4);
		block_entity_set_scale(e, scale_x, scale_y);
	}
	else if (strcmp(property, "block_id") == 0)
	{
		u64 block_id = (u64)luaL_checkinteger(L, 3);
		block_entity_set_block(e, block_id);
	}
	else if (strcmp(property, "var_handle") == 0)
	{
		handle32 handle = {
			.index = (u16)luaL_checkinteger(L, 3),
			.validation = (u16)luaL_checkinteger(L, 4),
			.active = 1,
		};
		block_entity_set_var_handle(e, handle);
	}
	else if (strcmp(property, "vars") == 0)
	{
		// setting vars would mean copying them, so we expect a VarHandle and copy the underlying blob
		blob *src_blob = get_blob_from_varhandle(L, 3);
		if (!src_blob)
		{
			luaL_error(L, "Expected VarHandle as value when setting 'vars' on BlockEntity");
			return 0;
		}
		handle32 newh = var_table_alloc_blob(&e->parent_layer->var_pool, *src_blob, false);
		if (newh.index == INVALID_HANDLE_INDEX)
		{
			luaL_error(L, "Failed to allocate new var blob when setting 'vars' on BlockEntity");
			return 0;
		}
		block_entity_set_var_handle(e, newh);
	}
	else
	{
		luaL_error(L, "Attempt to set unknown property '%s' on BlockEntity", property);
	}

	return 0;
}

static int entity_remove_function(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, BlockEntity, wrapper, 1);

	if (wrapper->l && handle_is_valid(wrapper->l->block_entity_pool, wrapper->h))
	{
		layer_remove_block_entity(wrapper->l, wrapper->h);
	}

	return 0;
}

static int lua_entity_index(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, BlockEntity, wrapper, 1);
	const char *key = luaL_checkstring(L, 2);

	block_entity *e = layer_get_block_entity(wrapper->l, wrapper->h);
	if (!e)
	{
		lua_pushnil(L);
		return 1;
	}
	else if (strcmp(key, "pos") == 0 || strcmp(key, "position") == 0)
	{
		lua_pushnumber(L, e->x);
		lua_pushnumber(L, e->y);
		return 2;
	}
	else if (strcmp(key, "vel") == 0 || strcmp(key, "velocity") == 0)
	{
		lua_pushnumber(L, e->vx);
		lua_pushnumber(L, e->vy);
		return 2;
	}
	else if (strcmp(key, "uuid") == 0)
	{
		lua_pushinteger(L, (lua_Integer)e->uuid);
		return 1;
	}
	else if (strcmp(key, "rotation") == 0)
	{
		lua_pushnumber(L, e->rotation);
		return 1;
	}
	else if (strcmp(key, "scale") == 0)
	{
		lua_pushnumber(L, e->scale_x);
		lua_pushnumber(L, e->scale_y);
		return 2;
	}
	else if (strcmp(key, "block_id") == 0)
	{
		lua_pushinteger(L, e->block_id);
		return 1;
	}
	else if (strcmp(key, "remove") == 0)
	{
		lua_pushcfunction(L, entity_remove_function);
		return 1;
	}
	else if (strcmp(key, "var_handle") == 0)
	{
		lua_pushinteger(L, e->var_handle.index);
		lua_pushinteger(L, e->var_handle.validation);
		return 2;
	}
	else if (strcmp(key, "vars") == 0)
	{
		if (handle_is_valid(e->parent_layer->var_pool.table, e->var_handle))
		{
			push_varhandle(L, e->parent_layer, e->var_handle);
		}
		else
		{
			lua_pushnil(L);
		}
		return 1;
	}
	else
	{
		luaL_error(L, "Attempt to access unknown property '%s' on BlockEntity", key);
	}

	lua_pushnil(L);
	return 1;
}

void lua_entity_register(lua_State *L)
{
	luaL_newmetatable(L, "BlockEntity");

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, lua_entity_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, lua_entity_new_index);
	lua_settable(L, -3);
}
