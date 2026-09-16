#include "include/scripting/block_component.h"

#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#include "include/block_registry.h"
#include "include/general.h"
#include "include/scripting.h"
#include "include/vars.h"

// all component bindings operate on the currently loading block resource
static block_resources *current_block()
{
	if (g_current_block_res == NULL)
		luaL_error(g_L, "component_* called outside of block script loading");
	return g_current_block_res;
}

static const char *single_letter(lua_State *L, int idx)
{
	const char *s = luaL_checkstring(L, idx);
	if (strlen(s) != 1)
		luaL_error(L, "expected a single-letter key, got \"%s\"", s);
	return s;
}

// component_add_var(letter, size)
static int lua_component_add_var(lua_State *L)
{
	const char *letter = single_letter(L, 1);
	i32 size = luaL_checkinteger(L, 2);

	block_resources *res = current_block();
	lua_pushboolean(L, var_add(&res->vars_sample, letter[0], (u8)size) == SUCCESS);
	return 1;
}

#define COMPONENT_SETTER_FN(type)                                                                                      \
	static int lua_component_set_##type(lua_State *L)                                                                  \
	{                                                                                                                  \
		const char *letter = single_letter(L, 1);                                                                      \
		lua_Number number = luaL_checknumber(L, 2);                                                                    \
		block_resources *res = current_block();                                                                        \
		lua_pushboolean(L, var_set_##type(&res->vars_sample, letter[0], (type)number) == SUCCESS);                     \
		return 1;                                                                                                      \
	}

#define COMPONENT_GETTER_FN(type, cname)                                                                               \
	static int lua_component_get_##cname(lua_State *L)                                                                 \
	{                                                                                                                  \
		const char *letter = single_letter(L, 1);                                                                      \
		block_resources *res = current_block();                                                                        \
		type ret = 0;                                                                                                  \
		if (var_get_##cname(res->vars_sample, letter[0], &ret) == SUCCESS)                                             \
			lua_pushinteger(L, ret);                                                                                   \
		else                                                                                                           \
			lua_pushnil(L);                                                                                            \
		return 1;                                                                                                      \
	}

COMPONENT_SETTER_FN(u8)
COMPONENT_SETTER_FN(u16)
COMPONENT_SETTER_FN(u32)
COMPONENT_SETTER_FN(u64)
COMPONENT_SETTER_FN(i8)
COMPONENT_SETTER_FN(i16)
COMPONENT_SETTER_FN(i32)
COMPONENT_SETTER_FN(i64)

COMPONENT_GETTER_FN(u8, u8)
COMPONENT_GETTER_FN(u16, u16)
COMPONENT_GETTER_FN(u32, u32)
COMPONENT_GETTER_FN(u64, u64)
COMPONENT_GETTER_FN(i8, i8)
COMPONENT_GETTER_FN(i16, i16)
COMPONENT_GETTER_FN(i32, i32)
COMPONENT_GETTER_FN(i64, i64)

// component_set_str(letter, value)
static int lua_component_set_str(lua_State *L)
{
	const char *letter = single_letter(L, 1);
	const char *value = luaL_checkstring(L, 2);

	block_resources *res = current_block();
	lua_pushboolean(L, var_set_str(&res->vars_sample, letter[0], value) == SUCCESS);
	return 1;
}

// component_get_str(letter)
static int lua_component_get_str(lua_State *L)
{
	const char *letter = single_letter(L, 1);

	block_resources *res = current_block();
	char *out = NULL;
	if (var_get_str(res->vars_sample, letter[0], &out) == SUCCESS)
		lua_pushstring(L, out);
	else
		lua_pushnil(L);
	return 1;
}

// component_set_controller(kind, letter)
// kind: "anim", "type", "flip", "rotation", "offset_x", "offset_y", "interp_timestamp"
static int lua_component_set_controller(lua_State *L)
{
	const char *kind = luaL_checkstring(L, 1);
	const char *letter = single_letter(L, 2);

	block_resources *res = current_block();
	char c = letter[0];

	if (strcmp(kind, "anim") == 0)
		res->anim_controller = c;
	else if (strcmp(kind, "type") == 0)
		res->type_controller = c;
	else if (strcmp(kind, "flip") == 0)
		res->flip_controller = c;
	else if (strcmp(kind, "rotation") == 0)
		res->rotation_controller = c;
	else if (strcmp(kind, "offset_x") == 0)
		res->offset_x_controller = c;
	else if (strcmp(kind, "offset_y") == 0)
		res->offset_y_controller = c;
	else if (strcmp(kind, "interp_timestamp") == 0)
		res->interp_timestamp_controller = c;
	else
		return luaL_error(L, "unknown controller kind \"%s\"", kind);

	lua_pushboolean(L, 1);
	return 1;
}

// component_set_interp_takes(ms)
static int lua_component_set_interp_takes(lua_State *L)
{
	block_resources *res = current_block();
	res->interp_takes = luaL_checkinteger(L, 1);
	lua_pushboolean(L, 1);
	return 1;
}

// component_register_api(name, func) registers into the API table of the
// current block so other components (same block or different block types) can
// reach it: scripting_block_apis[block_id].name
static int lua_component_register_api(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	luaL_checkany(L, 2);

	lua_getglobal(L, "scripting_current_block_api");
	if (lua_isnil(L, -1))
		return luaL_error(L, "component_register_api called outside of block script loading");

	lua_pushvalue(L, 2);
	lua_setfield(L, -2, name);

	lua_pushboolean(L, 1);
	return 1;
}

// component_get_block_api(block_id) returns the API table of another block
// type, or nil.
static int lua_component_get_block_api(lua_State *L)
{
	i64 block_id = luaL_checkinteger(L, 1);

	lua_getglobal(L, "scripting_block_apis");
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	lua_geti(L, -1, block_id);
	return 1;
}

static const struct luaL_Reg component_funcs[] = {
	{		 "component_add_var",		  lua_component_add_var},
	{		  "component_set_u8",			 lua_component_set_u8},
	{		 "component_set_u16",		  lua_component_set_u16},
	{		 "component_set_u32",		  lua_component_set_u32},
	{		 "component_set_u64",		  lua_component_set_u64},
	{		  "component_set_i8",			 lua_component_set_i8},
	{		 "component_set_i16",		  lua_component_set_i16},
	{		 "component_set_i32",		  lua_component_set_i32},
	{		 "component_set_i64",		  lua_component_set_i64},
	{		  "component_get_u8",			 lua_component_get_u8},
	{		 "component_get_u16",		  lua_component_get_u16},
	{		 "component_get_u32",		  lua_component_get_u32},
	{		 "component_get_u64",		  lua_component_get_u64},
	{		  "component_get_i8",			 lua_component_get_i8},
	{		 "component_get_i16",		  lua_component_get_i16},
	{		 "component_get_i32",		  lua_component_get_i32},
	{		 "component_get_i64",		  lua_component_get_i64},
	{		 "component_set_str",		  lua_component_set_str},
	{		 "component_get_str",		  lua_component_get_str},
	{	 "component_set_controller",	 lua_component_set_controller},
	{"component_set_interp_takes", lua_component_set_interp_takes},
	{	 "component_register_api",	   lua_component_register_api},
	{	 "component_get_block_api",	lua_component_get_block_api},
	{						NULL,						   NULL},
};

void lua_block_component_register(lua_State *L)
{
	luaL_newlib(L, component_funcs);
	lua_setglobal(L, "component");
}