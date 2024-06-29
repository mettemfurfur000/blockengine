#include "include/lua_integration.h"

lua_State *g_L = 0;

void scripting_init()
{
	g_L = luaL_newstate(); /* opens Lua */
	luaL_openlibs(g_L);
}

void scripting_close()
{
	lua_close(g_L);
}

int scripting_load_file(const char *filename)
{
	int error = luaL_loadfile(g_L, filename);
	if (error)
	{
		fprintf(stderr, "%s", lua_tostring(g_L, -1));
		return FAIL;
	}
	lua_pop(g_L, 1); /* pop error message from the stack */
	return SUCCESS;
}