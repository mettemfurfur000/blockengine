#include "include/scripting/sound.h"

#include "include/scripting.h"
#include <lua.h>

static int lua_sound_play(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Sound, wrapper, 1);

	lua_pushinteger(L, Mix_PlayChannel(-1, wrapper->s->obj, 0));

	return 1;
}

static int lua_sound_set_volume_chunk(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Sound, wrapper, 1);
	u8 volume = luaL_checkinteger(L, 2);

	lua_pushinteger(L, Mix_VolumeChunk(wrapper->s->obj, volume));

	return 1;
}

void lua_sound_register(lua_State *L)
{
	const static luaL_Reg sound_methods[] = {
		{		 "play",			 lua_sound_play},
		{"set_volume", lua_sound_set_volume_chunk},
		{		 NULL,					   NULL},
	};

	luaL_newmetatable(L, "Sound");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, sound_methods, 0);
}