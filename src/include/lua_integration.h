#ifndef LUA_INTEGRATION
#define LUA_INTEGRATION

#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "game_types.h"
#include "block_operations.h"

extern lua_State *g_L;

void scripting_init();

void scripting_close();

int scripting_load_file(const char *filename);

#endif