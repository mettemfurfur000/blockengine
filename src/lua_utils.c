#include "include/lua_utils.h"

int block_unpack(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 1 || !lua_isuserdata(L, 1))
    {
        lua_pushliteral(L, "expected a block");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    lua_pushinteger(L, b->id);
    lua_pushinteger(L, b->data ? b->data[0] : 0);

    return 2; /* number of results */
}

int get_keyboard_state(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 1 || !lua_istable(L, 1))
    {
        lua_pushliteral(L, "expected a table to fill");
        lua_error(L);
    }

    int len = 0;
    const Uint8 *keystate = SDL_GetKeyboardState(&len);

    for (int i = 0; i < len; i++)
    {
        lua_pushinteger(L, i);
        lua_pushboolean(L, keystate[i]);
        lua_settable(L, -3);
    }

    return 1;
}