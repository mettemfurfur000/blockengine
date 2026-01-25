#ifndef SCRIPTING_BINDINGS_H
#define SCRIPTING_BINDINGS_H 1

#include <stdint.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/*

#define LUA_FUNC_DEF(func_name) static int lua_##func_name(lua_State *L)
#define LUA_STACK_IDX_INIT() u32 __stack_idx = 1;
#define LUA_THIS(type) LUA_CHECK_USER_OBJECT(L, Layer, this, __stack_idx++)

#define LUA_GET_U8(name) u8 name = luaL_checkinteger(L, __stack_idx++);
#define LUA_GET_U16(name) u16 name = luaL_checkinteger(L, __stack_idx++);
#define LUA_GET_U32(name) u32 name = luaL_checkinteger(L, __stack_idx++);
#define LUA_GET_U64(name) u64 name = luaL_checkinteger(L, __stack_idx++);

// cant do that, thats UB
// #define GET_U8() luaL_checkinteger(L, __stack_idx++)
// #define GET_U16() luaL_checkinteger(L, __stack_idx++)
// #define GET_U32() luaL_checkinteger(L, __stack_idx++)
// #define GET_U64() luaL_checkinteger(L, __stack_idx++)

#define LUA_BOOLCALL_RET(func_call)                                                                                    \
    lua_pushboolean(L, (func_call) == SUCCESS);                                                                        \
    return 1;

#define LUA_SWITCH_CALL(func_call, success_call, failure_call)                                                         \
    if ((func_call) == SUCCESS)                                                                                        \
        success_call;                                                                                                  \
    else                                                                                                               \
        failure_call;                                                                                                  \
    return 1;
*/

void image_load_editing_library(lua_State *L);
void lua_register_render_rules(lua_State *L);

/*
The only userdata types we have here for now:

Level
Room
Layer

all, except for vars, has :uuid() to check for equality

*/

void lua_level_register(lua_State *L);
void lua_room_register(lua_State *L);
void lua_layer_register(lua_State *L);
void lua_level_editing_lib_register(lua_State *L);
// includes all above + some extra
void lua_register_engine_objects(lua_State *L);

// networking
void lua_network_register(lua_State *L);

void lua_entity_register(lua_State *L);

// some public lua functions

int lua_light_block_input_register(lua_State *L);

#endif