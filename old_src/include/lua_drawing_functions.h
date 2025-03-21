#ifndef LUA_DRAWING_FUNCTIONS_H
#define LUA_DRAWING_FUNCTIONS_H

#include "lua_integration.h"
#include "layer_draw_2d.h"

int lua_render_rules_get_resolutions(lua_State *L); // -> 2 numbers

int lua_render_rules_get_order(lua_State *L); // -> table{number}

int lua_slice_get(lua_State *L); // sets/gets a whole slice structure
int lua_slice_set(lua_State *L);

#endif