#include "include/scripting_bindings.h"

#include "include/block_registry.h"
#include "include/level.h"
#include "include/scripting.h"
#include "include/scripting/render_rules.h"
#include "include/scripting_var_handles.h"

#include "include/scripting/entity.h"
#include "include/scripting/image.h"
#include "include/scripting/level.h"
#include "include/scripting/registry.h"
#include "include/scripting/sound.h"

#include <lauxlib.h>
#include <lua.h>

int lua_uuid_shared(lua_State *L)
{
	void *ptr = 0;
	ptr = luaL_testudata(L, 1, "Level");
	if (ptr != 0)
	{
		lua_pushinteger(L, ((level *)ptr)->uuid);
		return 1;
	}
	ptr = luaL_testudata(L, 1, "Room");
	if (ptr != 0)
	{
		lua_pushinteger(L, ((room *)ptr)->uuid);
		return 1;
	}
	ptr = luaL_testudata(L, 1, "Layer");
	if (ptr)
	{
		lua_pushinteger(L, ((layer *)ptr)->uuid);
		return 1;
	}
	ptr = luaL_testudata(L, 1, "BlockRegistry");
	if (ptr)
	{
		lua_pushinteger(L, ((block_registry *)ptr)->uuid);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

static int lua_get_ticks(lua_State *L)
{
	lua_pushinteger(L, SDL_GetTicks());
	// lua_pushinteger(L, clock());
	return 1;
}

// maybe the only function that will be public to the rest of my codebase, since
// i need it at registry creation
int lua_light_block_input_register(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
		luaL_error(L, "Expected lightuserdata as first argument");
	void *ptr = lua_touserdata(L, 1);
	u64 id = luaL_checkinteger(L, 2);
	const char *name = luaL_checkstring(L, 3);
	luaL_checkany(L, 4);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushboolean(L, scripting_register_block_input((block_registry *)ptr, id, ref, name) == SUCCESS);
	return 1;
}

void lua_sdl_functions_register(lua_State *L)
{
	const static luaL_Reg sdl_methods[] = {
		{"get_ticks", lua_get_ticks},
		{		 NULL,		   NULL},
	};

	luaL_newlib(L, sdl_methods);
	lua_setglobal(L, "sdl");
}

void lua_register_engine_objects(lua_State *L)
{
	lua_sdl_functions_register(L);

	lua_level_register(L); /* level editing */
	lua_room_register(L);
	lua_layer_register(L);

	lua_entity_register(L);
	lua_block_registry_register(L);
	lua_sound_register(L);
	lua_varhandle_register(L);

	image_load_editing_library(L);
	
	/* client render rules */
	lua_logging_register(g_L);
	lua_level_editing_lib_register(g_L);
	lua_register_render_rules(g_L);

}
