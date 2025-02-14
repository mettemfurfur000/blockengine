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

#define LUA_RAISE_ERROR(L, errtext) \
    lua_pushliteral(L, errtext), lua_error(L);

#define STRUCT_GETFIELD(L, object, field, pusher) \
    pusher(L, object.field);                      \
    lua_setfield(L, -2, #field);

#define STRUCT_SETFIELD(L, object, field, expected_type, converter) \
    if (lua_getfield(L, -1, #field) == expected_type)               \
    {                                                               \
        object.field = converter(L, -1);                            \
        lua_pop(L, 1);                                              \
    }                                                               \
    else                                                            \
        LUA_RAISE_ERROR(L, "STRUCT_SETFIELD: expected a " #expected_type "for a field" #field)

#define STRUCT_CHECK(L, value, max_size) \
    if (index >= value)                  \
    LUA_RAISE_ERROR(L, #value " >= " #max_size)

#define LUA_ARGS_NUMBER_CHECK(L, mustbe) \
    if (lua_gettop(L) != mustbe)         \
    LUA_RAISE_ERROR(L, "expected " #mustbe " arguments")

#define LUA_ARG_CHECK(L, pos, mustbe) \
    if (lua_type(L, pos) != mustbe)   \
    LUA_RAISE_ERROR(L, "expected " #mustbe " at " #pos)

void scripting_check_arguments(lua_State *L, int num, ...);

extern lua_State *g_L;

typedef vec_t(lua_CFunction) vec_CFunction_t;

typedef struct
{
    int event_id;
    vec_void_t functions;
} event_handler;

void scripting_init();
void scripting_close();

void scripting_register_event(lua_CFunction function, const int event_id);
int scripting_handle_event(SDL_Event *event, const int override_id);

void scripting_load_scripts(block_registry *registry);

void scripting_define_global_object(void *ptr, char *name);

// lua functions

int lua_set_block_id(lua_State *L);
int lua_get_block_id(lua_State *L);

int lua_get_block_vars(lua_State *L);
int lua_vars_get_integer(lua_State *L);

int lua_vars_set_integer(lua_State *L);

// rendering things

int lua_render_rules_get_resolutions(lua_State *L);
int lua_render_rules_get_order(lua_State *L);

// unpacks data from a slice object
int lua_slice_get(lua_State *L);

// packs slice from a lua table back into c structure and then sets it in a render rules
int lua_slice_set(lua_State *L);

#endif