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

	else if (strcmp(property, "position_x") == 0)
		e->pos.x = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "position_y") == 0)
		e->pos.y = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "velocity_x") == 0)
		e->velocity.x = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "velocity_y") == 0)
		e->velocity.y = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "force_x") == 0)
		e->force.x = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "force_y") == 0)
		e->force.y = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "rotation") == 0)
		e->rotation = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "mass") == 0)
		e->mass = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "scale_x") == 0)
		e->scale_x = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "scale_y") == 0)
		e->scale_y = (float)luaL_checknumber(L, 3);
	else if (strcmp(property, "block_id") == 0)
		e->block_id = (u64)luaL_checkinteger(L, 3);
	else if (strcmp(property, "var_handle") == 0)
	{
		handle32 h = get_handle_from_varhandle(L, 3);
		block_entity_set_var_handle(e, h);
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
	else if (strcmp(key, "position_x") == 0)
	{
		lua_pushnumber(L, e->pos.x);
		return 1;
	}
	else if (strcmp(key, "position_y") == 0)
	{
		lua_pushnumber(L, e->pos.y);
		return 1;
	}
	else if (strcmp(key, "velocity_x") == 0)
	{
		lua_pushnumber(L, e->velocity.x);
		return 1;
	}
	else if (strcmp(key, "velocity_y") == 0)
	{
		lua_pushnumber(L, e->velocity.y);
		return 1;
	}
	else if (strcmp(key, "force_x") == 0)
	{
		lua_pushnumber(L, e->force.x);
		return 1;
	}
	else if (strcmp(key, "force_y") == 0)
	{
		lua_pushnumber(L, e->force.y);
		return 1;
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
	else if (strcmp(key, "scale_x") == 0)
	{
		lua_pushnumber(L, e->scale_x);
		return 1;
	}
	else if (strcmp(key, "scale_y") == 0)
	{
		lua_pushnumber(L, e->scale_y);
		return 1;
	}
	else if (strcmp(key, "block_id") == 0)
	{
		lua_pushinteger(L, e->block_id);
		return 1;
	}
	else if (strcmp(key, "mass") == 0)
	{
		lua_pushnumber(L, e->mass);
		return 1;
	}
	else if (strcmp(key, "remove") == 0)
	{
		lua_pushcfunction(L, entity_remove_function);
		return 1;
	}
	else if (strcmp(key, "vars") == 0 || strcmp(key, "var_handle") == 0)
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
