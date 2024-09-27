#ifndef LUA_BLOCK_DATA_FUNCTIONS_H
#define LUA_BLOCK_DATA_FUNCTIONS_H

#include "lua_integration.h"
#include "game_types.h"

#include "data_manipulations.h"

// memory

int lua_blob_create(lua_State *L);
int lua_blob_remove(lua_State *L);

// set

int lua_blob_set_str(lua_State *L);
int lua_blob_set_i(lua_State *L);
int lua_blob_set_s(lua_State *L);
int lua_blob_set_b(lua_State *L);

// get

int lua_blob_get_str(lua_State *L);
int lua_blob_get_i(lua_State *L);
int lua_blob_get_s(lua_State *L);
int lua_blob_get_b(lua_State *L);

#endif