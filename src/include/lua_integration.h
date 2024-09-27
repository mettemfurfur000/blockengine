#ifndef LUA_INTEGRATION
#define LUA_INTEGRATION

#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "game_types.h"
#include "block_operations.h"
#include "block_registry.h"

#include <SDL2/SDL_events.h>

extern lua_State *g_L;

/*
template for all integrated functions:

static int lua_(lua_State *L)
{
    int n = lua_gettop(L); // number of arguments
    return 0;
}
*/

typedef vec_t(lua_CFunction) vec_CFunction_t;

typedef struct
{
    int event_id;
    vec_void_t functions;
} event_handler;

void scripting_init();
void scripting_close();

void scripting_register_event(lua_CFunction function, const int event_id);
int scripting_handle_event(SDL_Event *event);

int scripting_load_file(const char *filename);
void scripting_load_scripts(block_registry_t *reg);
void scripting_define_global_variables(const world *w);

#endif