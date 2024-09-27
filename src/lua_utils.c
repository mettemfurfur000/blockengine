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