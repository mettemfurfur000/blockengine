#ifndef SCRIPTING_BINDINGS_H
#define SCRIPTING_BINDINGS_H 1

#include <stdint.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int lua_uuid_shared(lua_State *L);

void lua_register_engine_objects(lua_State *L);

int lua_light_block_input_register(lua_State *L);

#endif