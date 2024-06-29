#ifndef BLOCK_SCRIPTING
#define BLOCK_SCRIPTING

#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "game_types.h"
#include "block_updates.h"

extern lua_State *g_L;

void scripting_init();

void scripting_close();

int scripting_load_file(const char *filename);

#endif