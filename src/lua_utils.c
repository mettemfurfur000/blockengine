#include "include/lua_utils.h"

int block_unpack(lua_State *L)
{
    scripting_check_arguments(L, 1, LUA_TLIGHTUSERDATA);

    block *b = lua_touserdata(L, 1);
    lua_pushinteger(L, b->id);
    lua_pushinteger(L, b->data ? b->data[0] : 0);

    return 2; /* number of results */
}

int get_keyboard_state(lua_State *L)
{
    scripting_check_arguments(L, 1, LUA_TTABLE);

    int len = 0;
    const Uint8 *keystate = SDL_GetKeyboardState(&len);

    if (keystate == 0)
        return 1;

    for (int i = 0; i < len; i++)
    {
        lua_pushinteger(L, i);
        lua_pushboolean(L, keystate[i]);
        lua_settable(L, -3);
    }

    return 1;
}