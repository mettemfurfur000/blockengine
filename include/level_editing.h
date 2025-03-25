#ifndef LEVEL_EDITING_H
#define LEVEL_EDITING_H

#include "scripting.h"
#include "level.h"
#include "rendering.h"
#include "block_registry.h"

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