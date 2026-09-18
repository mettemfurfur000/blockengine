#include "include/data_io.h"
#include "include/logging.h"
#include "include/scripting/block_trait.h"

#include <lauxlib.h>
#include <lua.h>
#include <signal.h>
#include <string.h>

#include "include/block_registry.h"
#include "include/general.h"
#include "include/scripting.h"
#include "include/vars.h"

#include "include/hashtable.h"

// all trait bindings operate on the currently loading block resource
static block_resources *current_block()
{
	if (g_current_block_res == NULL)
		luaL_error(g_L, "* called outside of block script loading");
	return g_current_block_res;
}

static const char *single_letter(lua_State *L, int idx)
{
	const char *s = luaL_checkstring(L, idx);
	if (strlen(s) != 1)
		luaL_error(L, "expected a single-letter key, got \"%s\"", s);
	return s;
}

// trait_add_var(letter, size)
static int lua_trait_add_var(lua_State *L)
{
	const char *letter = single_letter(L, 1);
	i32 size = luaL_checkinteger(L, 2);

	block_resources *res = current_block();
	lua_pushboolean(L, var_add(&res->vars_sample, letter[0], (u8)size) == SUCCESS);
	return 1;
}

#define TRAIT_SETTER_FN(type)                                                                                          \
	static int lua_trait_set_##type(lua_State *L)                                                                      \
	{                                                                                                                  \
		const char *letter = single_letter(L, 1);                                                                      \
		lua_Number number = luaL_checknumber(L, 2);                                                                    \
		block_resources *res = current_block();                                                                        \
		lua_pushboolean(L, var_set_##type(&res->vars_sample, letter[0], (type)number) == SUCCESS);                     \
		return 1;                                                                                                      \
	}

#define TRAIT_GETTER_FN(type, cname)                                                                                   \
	static int lua_trait_get_##cname(lua_State *L)                                                                     \
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

TRAIT_SETTER_FN(u8)
TRAIT_SETTER_FN(u16)
TRAIT_SETTER_FN(u32)
TRAIT_SETTER_FN(u64)
TRAIT_SETTER_FN(i8)
TRAIT_SETTER_FN(i16)
TRAIT_SETTER_FN(i32)
TRAIT_SETTER_FN(i64)

TRAIT_GETTER_FN(u8, u8)
TRAIT_GETTER_FN(u16, u16)
TRAIT_GETTER_FN(u32, u32)
TRAIT_GETTER_FN(u64, u64)
TRAIT_GETTER_FN(i8, i8)
TRAIT_GETTER_FN(i16, i16)
TRAIT_GETTER_FN(i32, i32)
TRAIT_GETTER_FN(i64, i64)

// trait_set_str(letter, value)
static int lua_trait_set_str(lua_State *L)
{
	const char *letter = single_letter(L, 1);
	const char *value = luaL_checkstring(L, 2);

	block_resources *res = current_block();
	lua_pushboolean(L, var_set_str(&res->vars_sample, letter[0], value) == SUCCESS);
	return 1;
}

// trait_get_str(letter)
static int lua_trait_get_str(lua_State *L)
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

// trait_set_controller(kind, letter)
// kind: "anim", "type", "flip", "rotation", "offset_x", "offset_y", "interp_timestamp"
static int lua_trait_set_controller(lua_State *L)
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

// trait_set_interp_takes(ms)
static int lua_trait_set_interp_takes(lua_State *L)
{
	block_resources *res = current_block();
	res->interp_takes = luaL_checkinteger(L, 1);
	lua_pushboolean(L, 1);
	return 1;
}

// trait_register_api(name, func) registers into the API table of the
// current block so other traits (same block or different block types) can
// reach it: scripting_block_apis[block_id].name
static int lua_trait_register_api(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	luaL_checkany(L, 2);

	lua_getglobal(L, "scripting_current_block_api");
	if (lua_isnil(L, -1))
		return luaL_error(L, "register_api called outside of block script loading");

	lua_pushvalue(L, 2);
	lua_setfield(L, -2, name);

	lua_pushboolean(L, 1);
	return 1;
}

// trait_get_block_api(block_id) returns the API table of another block
// type, or nil.
static int lua_trait_get_block_api(lua_State *L)
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

static int lua_trait_get_block_id(lua_State *L)
{
	block_resources *res = current_block();

	lua_pushinteger(L, res->id);

	return 1;
}

static int lua_trait_get_field(lua_State *L)
{
	block_resources *res = current_block();

	const char *str = luaL_checkstring(L, 1);

	blob out = get_entry(res->all_fields, blobify(str));

	if (!out.ptr)
		lua_pushnil(L);
	else
		lua_pushstring(L, out.str);

	return 1;
}

static const struct luaL_Reg trait_funcs[] = {
	{		   "add_var",			lua_trait_add_var},
	{			"set_u8",		   lua_trait_set_u8},
	{		   "set_u16",			lua_trait_set_u16},
	{		   "set_u32",			lua_trait_set_u32},
	{		   "set_u64",			lua_trait_set_u64},
	{			"set_i8",		   lua_trait_set_i8},
	{		   "set_i16",			lua_trait_set_i16},
	{		   "set_i32",			lua_trait_set_i32},
	{		   "set_i64",			lua_trait_set_i64},
	{			"get_u8",		   lua_trait_get_u8},
	{		   "get_u16",			lua_trait_get_u16},
	{		   "get_u32",			lua_trait_get_u32},
	{		   "get_u64",			lua_trait_get_u64},
	{			"get_i8",		   lua_trait_get_i8},
	{		   "get_i16",			lua_trait_get_i16},
	{		   "get_i32",			lua_trait_get_i32},
	{		   "get_i64",			lua_trait_get_i64},
	{		   "set_str",			lua_trait_set_str},
	{		   "get_str",			lua_trait_get_str},
	{	 "set_controller",   lua_trait_set_controller},
	{	 "set_interp_takes", lua_trait_set_interp_takes},
	{	  "register_api",	 lua_trait_register_api},
	{	 "get_block_api",	  lua_trait_get_block_api},
	{	  "get_block_id",	 lua_trait_get_block_id},
	{"get_resource_field",		   lua_trait_get_field},
	{				NULL,					   NULL},
};

void lua_block_trait_register(lua_State *L)
{
	luaL_newlib(L, trait_funcs);
	lua_setglobal(L, "trait");
}