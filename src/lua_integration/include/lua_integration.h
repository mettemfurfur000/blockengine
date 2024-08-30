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

extern lua_State *g_L;

/*
template for all integrated functions:

static int lua_(lua_State *L)
{
    int n = lua_gettop(L); // number of arguments
    return 0;
}
*/

void scripting_init();

void scripting_close();

int scripting_load_file(const char *filename);

#endif