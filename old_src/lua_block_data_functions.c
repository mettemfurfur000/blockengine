#include "include/lua_block_data_functions.h"

int lua_blob_create(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TSTRING, LUA_TNUMBER);

    block *b = lua_touserdata(L, 1);
    char letter = lua_tostring(L, 2)[0];
    int size = lua_tointeger(L, 3);

    data_create_element(&b->data, letter, size);

    return 0; /* number of results */
}

int lua_blob_remove(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TSTRING);

    block *b = lua_touserdata(L, 1);
    char letter = lua_tostring(L, 2)[0];

    data_delete_element(&b->data, letter);

    return 0; /* number of results */
}

// set

int lua_blob_set_str(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TSTRING, LUA_TSTRING);

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    const char *str = lua_tostring(L, 3);

    int result = data_set_str(b, letter, (const byte *)str, strlen(str));

    lua_pushboolean(L, result);

    return 1; /* number of results */
}

int lua_blob_set_i(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TSTRING, LUA_TNUMBER);

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];
    int value = lua_tointeger(L, 3);

    int result = data_set_i(b, letter, value);

    lua_pushboolean(L, result);

    return 1; /* number of results */
}

int lua_blob_set_s(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TSTRING, LUA_TNUMBER);

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    int number = lua_tointeger(L, 3);

    int result = data_set_s(b, letter, number);

    lua_pushboolean(L, result);

    return 1; /* number of results */
}
int lua_blob_set_b(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TSTRING, LUA_TNUMBER);

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
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TSTRING);

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
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TSTRING);

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
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TSTRING);

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
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TSTRING);

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    byte output;
    if (data_get_b(b, letter, &output) == SUCCESS)
        lua_pushinteger(L, output);
    else
        lua_pushnil(L);

    return 1; /* number of results */
}

int lua_blob_get_number(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TSTRING);

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    long long output;
    if (data_get_number(b, letter, &output) == SUCCESS)
        lua_pushinteger(L, output);
    else
        lua_pushnil(L);

    return 1; /* number of results */
}
int lua_blob_set_number(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TSTRING, LUA_TNUMBER);

    block *b = lua_touserdata(L, 1);
    const char letter = lua_tostring(L, 2)[0];

    long long number = lua_tointeger(L, 3);
    int result = data_set_number(b, letter, number);

    lua_pushboolean(L, result);

    return 1; /* number of results */
}