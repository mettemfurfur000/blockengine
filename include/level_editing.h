#ifndef LEVEL_EDITING_H
#define LEVEL_EDITING_H

#include "../include/scripting.h"
#include "../include/level.h"
#include "../include/rendering.h"
#include "../include/block_registry.h"

/*
The only userdata types we have here:

Vars
Level
Room
Layer

BlockRegistry

*/

void lua_level_register(lua_State *L);
void lua_room_register(lua_State *L);
void lua_layer_register(lua_State *L);
void lua_vars_register(lua_State *L);

void lua_level_editing_lib_register(lua_State *L);

#endif