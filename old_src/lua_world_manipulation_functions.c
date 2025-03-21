#include "include/lua_world_manipulation_functions.h"

int lua_access_block(lua_State *L)
{
    scripting_check_arguments(L, 4, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER);

    world *w = (world *)lua_touserdata(L, 1);

    int layer_index = lua_tonumber(L, 2);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);
    block *b = get_block_access(w, layer_index, x, y);
    if (b == NULL)
        lua_pushnil(L);
    else
        lua_pushlightuserdata(L, b);

    return 1; /* number of results */
}

int lua_set_block(lua_State *L)
{
    scripting_check_arguments(L, 5, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TLIGHTUSERDATA);

    world *w = (world *)lua_touserdata(L, 1);
    int layer_index = lua_tonumber(L, 2);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);
    block *b = (block *)lua_touserdata(L, 5);
    int ret = set_block(w, layer_index, x, y, b);

    lua_pushboolean(L, ret);
    return 1;
}

int lua_clean_block(lua_State *L)
{
    scripting_check_arguments(L, 4, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER);

    world *w = (world *)lua_touserdata(L, 1);
    int layer_index = lua_tonumber(L, 2);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);

    int ret = clean_block(w, layer_index, x, y);

    lua_pushboolean(L, ret);
    return 1;
}

int lua_move_block_gently(lua_State *L)
{
    scripting_check_arguments(L, 6, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER);

    world *w = (world *)lua_touserdata(L, 1);
    int layer_index = lua_tonumber(L, 2);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);
    int v_x = lua_tonumber(L, 5);
    int v_y = lua_tonumber(L, 6);

    int ret = move_block_gently(w, layer_index, x, y, v_x, v_y);

    lua_pushboolean(L, ret);
    return 1;
}

int lua_move_block_rough(lua_State *L)
{
    scripting_check_arguments(L, 6, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER);

    world *w = (world *)lua_touserdata(L, 1);
    int layer_index = lua_tonumber(L, 2);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);
    int v_x = lua_tonumber(L, 5);
    int v_y = lua_tonumber(L, 6);

    int ret = move_block_rough(w, layer_index, x, y, v_x, v_y);

    lua_pushboolean(L, ret);
    return 1;
}

int lua_move_block_recursive(lua_State *L)
{
    scripting_check_arguments(L, 7, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER);

    world *w = (world *)lua_touserdata(L, 1);
    int layer_index = lua_tonumber(L, 2);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);
    int v_x = lua_tonumber(L, 5);
    int v_y = lua_tonumber(L, 6);
    int recursive_level = lua_tonumber(L, 7);

    int ret = move_block_recursive(w, layer_index, x, y, v_x, v_y, recursive_level);

    lua_pushboolean(L, ret);
    return 1;
}

int lua_is_data_equal(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TLIGHTUSERDATA);

    block *a = (block *)lua_touserdata(L, 1);
    block *b = (block *)lua_touserdata(L, 2);

    lua_pushboolean(L, is_data_equal(a, b));
    return 1; /* number of results */
}

int lua_is_block_void(lua_State *L)
{
    scripting_check_arguments(L, 1, LUA_TLIGHTUSERDATA);

    lua_pushboolean(L, is_block_void((block *)lua_touserdata(L, 1)));
    return 1; /* number of results */
}

int lua_is_block_equal(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TLIGHTUSERDATA);

    block *a = (block *)lua_touserdata(L, 1);
    block *b = (block *)lua_touserdata(L, 2);

    lua_pushboolean(L, is_block_equal(a, b));
    return 1; /* number of results */
}

int lua_is_chunk_equal(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TLIGHTUSERDATA);

    layer_chunk *a = (layer_chunk *)lua_touserdata(L, 1);
    layer_chunk *b = (layer_chunk *)lua_touserdata(L, 2);
    lua_pushboolean(L, is_chunk_equal(a, b));

    return 1; /* number of results */
}

int lua_block_data_free(lua_State *L)
{
    scripting_check_arguments(L, 1, LUA_TLIGHTUSERDATA);

    block_data_free((block *)lua_touserdata(L, 1));

    return 0; /* number of results */
}

int lua_block_erase(lua_State *L)
{
    scripting_check_arguments(L, 1, LUA_TLIGHTUSERDATA);

    block_erase((block *)lua_touserdata(L, 1));

    return 0; /* number of results */
}

int lua_block_copy(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TLIGHTUSERDATA);

    block_copy((block *)lua_touserdata(L, 1), (block *)lua_touserdata(L, 2));
    return 0; /* number of results */
}

/*
    1st argument is block pointer,
    2nd is block id,
    3nd is formatted data string from block registry header
*/
int lua_block_init(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TSTRING);

    block *b = (block *)lua_touserdata(L, 1);
    int id = lua_tonumber(L, 2);
    block_init(b, id, 0, 0);

    int ret = make_block_data_from_string(lua_tostring(L, 3), &b->data);

    if (ret == FAIL)
    {
        lua_pushliteral(L, "Failed to create block data from string");
        lua_error(L);
    }

    return 0; /* number of results */
}

int lua_block_teleport(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TLIGHTUSERDATA);

    block_teleport((block *)lua_touserdata(L, 1), (block *)lua_touserdata(L, 2));
    return 0; /* number of results */
}

int lua_block_swap(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TLIGHTUSERDATA);

    block_swap((block *)lua_touserdata(L, 1), (block *)lua_touserdata(L, 2));
    return 0; /* number of results */
}