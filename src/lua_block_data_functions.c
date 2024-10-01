#include "include/lua_block_data_functions.h"

int lua_blob_create(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 3 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2) ||
        !lua_isinteger(L, 3))
    {
        lua_pushliteral(L, "expected a block, a letter, and size");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    char letter = lua_tostring(L, 2)[0];
    int size = lua_tointeger(L, 3);

    data_create_element(&b->data, letter, size);

    return 0; /* number of results */
}

int lua_blob_remove(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2))
    {
        lua_pushliteral(L, "expected a block and a letter");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    char letter = lua_tostring(L, 2)[0];

    data_delete_element(&b->data, letter);

    return 0; /* number of results */
}

// set

int lua_blob_set_str(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2) ||
        !lua_isstring(L, 3))
    {
        lua_pushliteral(L, "expected a block, a letter and a string");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    const char *str = lua_tostring(L, 3);

    int result = data_set_str(b, letter, (const byte *)str, strlen(str));

    lua_pushboolean(L, result);

    return 1; /* number of results */
}

int lua_blob_set_i(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2) ||
        !(lua_isinteger(L, 3) || lua_isnumber(L, 3)))
    {
        lua_pushliteral(L, "expected a block, a letter and integer / number");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    int number = lua_tointeger(L, 3);

    int result = data_set_i(b, letter, number);

    lua_pushboolean(L, result);

    return 1; /* number of results */
}

int lua_blob_set_s(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2) ||
        !(lua_isinteger(L, 3) || lua_isnumber(L, 3)))
    {
        lua_pushliteral(L, "expected a block, a letter and integer / number");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    int number = lua_tointeger(L, 3);

    int result = data_set_s(b, letter, number);

    lua_pushboolean(L, result);

    return 1; /* number of results */
}
int lua_blob_set_b(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2) ||
        !(lua_isinteger(L, 3) || lua_isnumber(L, 3)))
    {
        lua_pushliteral(L, "expected a block, a letter and integer / number");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    int number = lua_tointeger(L, 3);

    int result = data_set_b(b, letter, number);

    lua_pushboolean(L, result);

    return 1; /* number of results */
}

// get

int lua_blob_get_str(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2))
    {
        lua_pushliteral(L, "expected a block and a letter");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    char buf[256];
    if (data_get_str(b, letter, (byte *)buf, sizeof(buf)) == SUCCESS)
        lua_pushstring(L, buf);
    else
        lua_pushnil(L);

    return 1; /* number of results */
}

int lua_blob_get_i(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2))
    {
        lua_pushliteral(L, "expected a block and a letter");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    int output;
    if (data_get_i(b, letter, &output) == SUCCESS)
        lua_pushinteger(L, output);
    else
        lua_pushnil(L);

    return 1; /* number of results */
}

int lua_blob_get_s(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2))
    {
        lua_pushliteral(L, "expected a block and a letter");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    short output;
    if (data_get_s(b, letter, &output) == SUCCESS)
        lua_pushinteger(L, output);
    else
        lua_pushnil(L);

    return 1; /* number of results */
}

int lua_blob_get_b(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 ||
        !lua_isuserdata(L, 1) ||
        !lua_isstring(L, 2))
    {
        lua_pushliteral(L, "expected a block and a letter");
        lua_error(L);
    }

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    byte output;
    if (data_get_b(b, letter, &output) == SUCCESS)
        lua_pushinteger(L, output);
    else
        lua_pushnil(L);

    return 1; /* number of results */
}
