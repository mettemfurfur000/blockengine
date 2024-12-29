#ifndef LUA_WORLD_MANIPULATION_FUNCTIONS_H
#define LUA_WORLD_MANIPULATION_FUNCTIONS_H

#include "lua_integration.h"

#include "engine_types.h"
#include "block_operations.h"
#include "block_registry.h"

int lua_access_block(lua_State *L);
int lua_set_block(lua_State *L);
int lua_clean_block(lua_State *L);

int lua_move_block_gently(lua_State *L);
int lua_move_block_rough(lua_State *L);
int lua_move_block_recursive(lua_State *L);

int lua_is_data_equal(lua_State *L);
int lua_is_block_void(lua_State *L);
int lua_is_block_equal(lua_State *L);

int lua_is_chunk_equal(lua_State *L);

int lua_block_data_free(lua_State *L);
int lua_block_erase(lua_State *L);
int lua_block_copy(lua_State *L);
int lua_block_init(lua_State *L);
int lua_block_teleport(lua_State *L);
int lua_block_swap(lua_State *L);

#endif