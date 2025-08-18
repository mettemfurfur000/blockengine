#ifndef SCRIPTING_BINDINGS_H
#define SCRIPTING_BINDINGS_H 1

#include <stdint.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>


void load_image_editing_library(lua_State *L);
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
void lua_vars_register(lua_State *L);
void lua_level_editing_lib_register(lua_State *L);
// includes all above + some extra
void lua_register_engine_objects(lua_State *L);

// some public lua functions

int lua_light_block_input_register(lua_State *L);

#endif