#ifndef LUA_INTEGRATION
#define LUA_INTEGRATION

#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "engine_types.h"
#include "block_operations.h"
#include "block_registry.h"

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

int scripting_load_file(const char *filename);
void scripting_load_scripts(block_registry_t *reg);

void scripting_define_global_object(void *ptr, char *name);

#endif