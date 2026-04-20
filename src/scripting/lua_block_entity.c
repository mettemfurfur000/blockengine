#include "include/scripting/entity.h"

#include "include/block_entity.h"
#include "include/handle.h"
#include "include/scripting.h"

#include "include/level.h"
#include "include/scripting_var_handles.h"
#include "include/vec_math.h"
#include <box2d/box2d.h>
#include <box2d/math_functions.h>
#include <lua.h>

static int lua_entity_new_index(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, BlockEntity, wrapper, 1);
	const char *property = luaL_checkstring(L, 2);

	block_entity *e = layer_get_block_entity(wrapper->l, wrapper->h);
	if (!e)
		return 0;

	else if (strcmp(property, "transform") == 0)
	{
		f32 x = lua_getfield(L, 3, "x");
		f32 y = lua_getfield(L, 3, "y");
		f32 c = lua_getfield(L, 3, "c");
		f32 s = lua_getfield(L, 3, "s");

		b2Transform t = {
			{x, y},
			{c, s}
		  };

		b2Body_SetTransform(e->b2_body_id, t.p, t.q);
	}
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

#define LUA_ENT_PRE_CALL_CHECKS(L)                                                                                     \
	LUA_CHECK_USER_OBJECT(L, BlockEntity, wrapper, 1);                                                                 \
	assert(wrapper->l);                                                                                                \
	if (!handle_is_valid(wrapper->l->block_entity_pool, wrapper->h))                                                   \
		return luaL_error(L, "Entity handle is invalid");

static int entity_remove_function(lua_State *L)
{
	LUA_ENT_PRE_CALL_CHECKS(L)

	return layer_remove_block_entity(wrapper->l, wrapper->h), 0;
}

// static int entity_apply_force_function(lua_State *L)
// {
// 	LUA_ENT_PRE_CALL_CHECKS(L)

// 	return layer_remove_block_entity(wrapper->l, wrapper->h), 0;
// }

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
	else if (strcmp(key, "transform") == 0)
	{
		if (!b2Body_IsValid(e->b2_body_id))
		{
			lua_pushnil(L);
			return 1;
		}

		b2Transform t = b2Body_GetTransform(e->b2_body_id);
		lua_newtable(L); // index 3

		lua_pushnumber(L, t.p.x);
		lua_setfield(L, 3, "x");
		lua_pushnumber(L, t.p.y);
		lua_setfield(L, 3, "y");

		// lua_pushnumber(L, t.q.c);
		// lua_setfield(L, 3, "c");
		// lua_pushnumber(L, t.q.s);
		// lua_setfield(L, 3, "s");

		lua_pushnumber(L, RAD_TO_DEG(b2Rot_GetAngle(t.q)));
		lua_setfield(L, 3, "r");

		return 1;
	}
	else if (strcmp(key, "uuid") == 0)
	{
		lua_pushinteger(L, (lua_Integer)e->uuid);
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
