#ifndef SCRIPTING_H
#define SCRIPTING_H

// #include <stdio.h>
// #include <string.h>
// #include "events.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "block_registry.h"
#include "level.h"

#include <SDL2/SDL_events.h>

typedef struct LuaHolder
{
    union
    {
        void *ptr;
        block_registry *reg;
        level *lvl;
        room *r;
        layer *l;
        blob *b;
        sound *s;
    };

} LuaHolder;

#define LUA_EXPECT_UNSIGNED(L)                                                 \
    if (lua_tointeger(L, -1) < 0)                                              \
        luaL_error(L, "STRUCT_SET: expected an unsigned integer for a field");

#define LUA_CHECK_USER_OBJECT(L, type, name, index)                            \
    LuaHolder *name = (LuaHolder *)luaL_checkudata(L, index, #type);

#define NEW_USER_OBJECT(L, type, pointer)                                      \
    ((LuaHolder *)lua_newuserdata(L, sizeof(void *)))->ptr = (pointer);        \
    luaL_getmetatable(L, #type);                                               \
    lua_setmetatable(L, -2);

#define LUA_SET_GLOBAL_OBJECT(name, ptr)                                       \
    {                                                                          \
        lua_pushlightuserdata(g_L, ptr);                                       \
        lua_setglobal(g_L, name);                                              \
    }

#define STRUCT_GET(L, object, field, pusher)                                   \
    pusher(L, object.field);                                                   \
    lua_setfield(L, -2, #field);

#define STRUCT_SET(L, object, field, expected_type, converter)                 \
    if (lua_getfield(L, -1, #field) == expected_type)                          \
    {                                                                          \
        object.field = converter(L, -1);                                       \
        lua_pop(L, 1);                                                         \
    } else                                                                     \
        luaL_error(L, "STRUCT_SET: expected a " #expected_type                 \
                      " for a field " #field)

extern lua_State *g_L;

// utils

void *check_light_userdata(lua_State *L, int index);

void scripting_init();
void scripting_close();

void call_handlers(SDL_Event e);
void scripting_register_event_handler(int ref, int event_type);

u8 scripting_load_scripts(block_registry *registry);
int scripting_load_file(const char *reg_name, const char *short_filename);

u8 scripting_register_block_input(block_registry *reg, u64 id, int ref,
                                  const char *name);

typedef struct enum_entry
{
    const char *enum_entry_name;
    u64 entry;
} enum_entry;

void scripting_set_global_enum(lua_State *L, enum_entry entries[],
                               const char *name);

#endif