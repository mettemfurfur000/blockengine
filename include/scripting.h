#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "block_registry.h"
#include "events.h"
#include "level.h"

#include <SDL2/SDL_events.h>

typedef struct LuaHolder
{
    union
    {
        void *ptr;
        level *lvl;
        room *r;
        layer *l;
        blob *b;
    };

} LuaHolder;

#define LUA_CHECK_USER_OBJECT(L, type, name, index) \
    LuaHolder *name = (LuaHolder *)luaL_checkudata(L, index, #type);

#define NEW_USER_OBJECT(L, type, pointer)                               \
    ((LuaHolder *)lua_newuserdata(L, sizeof(void *)))->ptr = (pointer); \
    luaL_getmetatable(L, #type);                                        \
    lua_setmetatable(L, -2);

#define LUA_SET_GLOBAL_OBJECT(name, ptr) \
    {                                    \
        lua_pushlightuserdata(g_L, ptr); \
        lua_setglobal(g_L, name);        \
    }

#define STRUCT_GET(L, object, field, pusher) \
    pusher(L, object.field);                 \
    lua_setfield(L, -2, #field);

#define STRUCT_SET(L, object, field, expected_type, converter) \
    if (lua_getfield(L, -1, #field) == expected_type)          \
    {                                                          \
        object.field = converter(L, -1);                       \
        lua_pop(L, 1);                                         \
    }                                                          \
    else                                                       \
        luaL_error(L, "STRUCT_SET: expected a " #expected_type " for a field " #field)

extern lua_State *g_L;

// utils

void *check_light_userdata(lua_State *L, int index);

void scripting_init();
void scripting_close();

void call_handlers(SDL_Event e);
void scripting_register_event_handler(int lua_func_ref, int event_type);

void scripting_load_scripts(block_registry *registry);
int scripting_load_file(const char *reg_name, const char *short_filename);

#endif