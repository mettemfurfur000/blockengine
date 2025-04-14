#ifndef SCRIPTING_BINDINGS_H
#define SCRIPTING_BINDINGS_H 1

#include "scripting.h"
#include "rendering.h"
#include "level.h"
#include "rendering.h"
#include "block_registry.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

void load_image_editing_library(lua_State *L);
void lua_register_render_rules(lua_State *L);

/*
The only userdata types we have here for now:

Vars
Level
Room
Layer

all, except for vars, has :uuid() to check for equality

*/

void lua_level_register(lua_State *L);
void lua_room_register(lua_State *L);
void lua_layer_register(lua_State *L);
void lua_vars_register(lua_State *L);
void lua_level_editing_lib_register(lua_State *L);
// includes all above + some extra
void lua_register_engine_objects(lua_State *L);

// some public lua functions

int lua_light_block_input_register(lua_State *L);

#define DECLARE_LUA_VAR_SETTER(type)                                                     \
    static int lua_var_set_##type(lua_State *L)                                          \
    {                                                                                    \
        LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);                                      \
        const char *key = luaL_checkstring(L, 2);                                        \
        lua_Number number = luaL_checknumber(L, 3);                                      \
        if (strlen(key) > 1)                                                             \
            luaL_error(L, "Key must be a single character");                             \
        lua_pushboolean(L, var_set_##type(wrapper->b, key[0], (type)number) == SUCCESS); \
        return 1;                                                                        \
    }

#define DECLARE_LUA_VAR_GETTER(type)                              \
    static int lua_var_get_##type(lua_State *L)                   \
    {                                                             \
        LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);               \
        const char *key = luaL_checkstring(L, 2);                 \
        type ret = 0;                                             \
        if (strlen(key) > 1)                                      \
            luaL_error(L, "Key must be a single character");      \
        if (var_get_##type(*wrapper->b, key[0], &ret) == SUCCESS) \
            lua_pushinteger(L, ret);                              \
        else                                                      \
            lua_pushnil(L);                                       \
        return 1;                                                 \
    }

#define STR(x) #x
#define LUA_VAR_OP_NAME(type, op) lua_var_##op##_##type
#define LUA_VAR_OP_FUNC_NAME(type, op) STR(op##_##type)
#define SCRIPTING_RECORD(type,op) {LUA_VAR_OP_FUNC_NAME(type,op), LUA_VAR_OP_NAME(type,op)}

#endif