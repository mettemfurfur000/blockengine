#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "block_registry.h"
#include "events.h"

#include <SDL2/SDL_events.h>

#define LUA_SET_GLOBAL_OBJECT(name, ptr) \
    {                                    \
        lua_pushlightuserdata(g_L, ptr); \
        lua_setglobal(g_L, name);        \
    }

#define STRUCT_GET(L, object, field, pusher) \
    pusher(L, object.field);                 \
    lua_setfield(L, -2, #field);

#define STRUCT_SET(L, object, field, converter) \
    object.field = converter(L, -1);            \
    lua_pop(L, 1);

extern lua_State *g_L;

// utils

void *check_light_userdata(lua_State *L, int index);

void scripting_init();
void scripting_close();

void call_handlers(SDL_Event e);
void scripting_register_event_handler(int lua_func_ref, int event_type);

void scripting_load_scripts(block_registry *registry);
int scripting_load_file(const char *reg_name, const char *short_filename);

// lua functions

int lua_register_handler(lua_State *L);

int lua_block_set_id(lua_State *L);
int lua_block_get_id(lua_State *L);

int lua_block_get_vars(lua_State *L);
int lua_block_set_vars(lua_State *L);

int lua_vars_get_integer(lua_State *L);
int lua_vars_set_integer(lua_State *L);

// rendering things

int lua_render_rules_get_resolutions(lua_State *L);

int lua_render_rules_get_order(lua_State *L);

int lua_slice_get(lua_State *L);
int lua_slice_set(lua_State *L);

// level managing functions

int lua_create_level(lua_State *L);

int lua_load_registry(lua_State *L);

int lua_create_room(lua_State *L);
int lua_get_room(lua_State *L);

int lua_get_room_count(lua_State *L);
int lua_create_layer(lua_State *L);

#endif