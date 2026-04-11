#ifndef SCRIPTING_LEVEL_H
#define SCRIPTING_LEVEL_H

#include <lua.h>

void lua_layer_register(lua_State *L);
void lua_level_register(lua_State *L);
void lua_room_register(lua_State *L);
void lua_level_editing_lib_register(lua_State *L);

#endif