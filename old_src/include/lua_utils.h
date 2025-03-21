#ifndef LUA_UTILS_H
#define LUA_UTILS_H

#include "lua_integration.h"
#include "engine_types.h"

int block_unpack(lua_State *L);
int get_keyboard_state(lua_State *L);

#endif