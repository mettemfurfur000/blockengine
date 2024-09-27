#include "include/lua_world_manipulation_functions.h"

int lua_access_block(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 4 || !lua_isuserdata(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
    {
        lua_pushliteral(L, "expected world, layer and x, y");
        lua_error(L);
    }

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
    int n = lua_gettop(L); /* number of arguments */

    if (n != 5 || !lua_isuserdata(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isuserdata(L, 5))
    {
        lua_pushliteral(L, "expected world, layer, x, y, and block to copy from");
        lua_error(L);
    }

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
    int n = lua_gettop(L); /* number of arguments */

    if (n != 4 || !lua_isuserdata(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
    {
        lua_pushliteral(L, "expected world, layer, and x, y");
        lua_error(L);
    }

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
    int n = lua_gettop(L); /* number of arguments */

    if (n != 6 || !lua_isuserdata(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5) || !lua_isnumber(L, 6))
    {
        lua_pushliteral(L, "expected world, layer, x, y, v2, v2");
        lua_error(L);
    }

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
    int n = lua_gettop(L); /* number of arguments */

    if (n != 6 || !lua_isuserdata(L, 1) || !lua_isinteger(L, 2) || !lua_isinteger(L, 3) || !lua_isinteger(L, 4) || !lua_isinteger(L, 5) || !lua_isinteger(L, 6))
    {
        lua_pushliteral(L, "expected world, layer, x, y, vx, vy");
        lua_error(L);
    }

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
    int n = lua_gettop(L); /* number of arguments */

    if (n != 7 || !lua_isuserdata(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5) || !lua_isnumber(L, 6) || !lua_isnumber(L, 7))
    {
        lua_pushliteral(L, "expected world, layer, x, y, v2, v2");
        lua_error(L);
    }

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
    int n = lua_gettop(L); /* number of arguments */
    int equal = SUCCESS;

    for (int i = 1; i < n; i++)
    {
        if (!lua_isuserdata(L, i) || !lua_isuserdata(L, i + 1))
        {
            lua_pushliteral(L, "expected userdata");
            lua_error(L);
        }
        block *a = (block *)lua_touserdata(L, i);
        block *b = (block *)lua_touserdata(L, i + 1);
        equal &= is_data_equal(a, b);
    }

    lua_pushboolean(L, equal);
    return 1; /* number of results */
}

int lua_is_block_void(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 1 || !lua_isuserdata(L, 1))
    {
        lua_pushliteral(L, "expected just 1 block");
        lua_error(L);
    }

    lua_pushboolean(L, is_block_void((block *)lua_touserdata(L, 1)));
    return 1; /* number of results */
}

int lua_is_block_equal(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 || !lua_isuserdata(L, 1) || !lua_isuserdata(L, 2))
    {
        lua_pushliteral(L, "expected 2 blocks");
        lua_error(L);
    }

    block *a = (block *)lua_touserdata(L, 1);
    block *b = (block *)lua_touserdata(L, 2);

    lua_pushboolean(L, is_block_equal(a, b));
    return 1; /* number of results */
}

int lua_is_chunk_equal(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 || !lua_isuserdata(L, 1) || !lua_isuserdata(L, 2))
    {
        lua_pushliteral(L, "expected 2 chunks");
        lua_error(L);
    }
    layer_chunk *a = (layer_chunk *)lua_touserdata(L, 1);
    layer_chunk *b = (layer_chunk *)lua_touserdata(L, 2);
    lua_pushboolean(L, is_chunk_equal(a, b));

    return 1; /* number of results */
}

int lua_block_data_free(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 1 || !lua_isuserdata(L, 1))
    {
        lua_pushliteral(L, "expected just 1 block");
        lua_error(L);
    }

    block_data_free((block *)lua_touserdata(L, 1));

    return 0; /* number of results */
}

int lua_block_erase(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 1 || !lua_isuserdata(L, 1))
    {
        lua_pushliteral(L, "expected just 1 block");
        lua_error(L);
    }

    block_erase((block *)lua_touserdata(L, 1));

    return 0; /* number of results */
}

int lua_block_copy(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 || !lua_isuserdata(L, 1) || !lua_isuserdata(L, 2))
    {
        lua_pushliteral(L, "expected 2 blocks");
        lua_error(L);
    }

    block_copy((block *)lua_touserdata(L, 1), (block *)lua_touserdata(L, 2));
    return 0; /* number of results */
}

int lua_block_init(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    /*
        1st argument is block pointer,
        2nd is block id,
        3nd is formatted data string from block registry header
    */

    if (n != 3 || !lua_isuserdata(L, 1) || !lua_isnumber(L, 2) || !lua_isstring(L, 3))
    {
        lua_pushliteral(L, "expected 3 arguments: userdata block pointer, block id, and a formatted data string");
        lua_error(L);
    }

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
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 || !lua_isuserdata(L, 1) || !lua_isuserdata(L, 2))
    {
        lua_pushliteral(L, "expected 2 blocks");
        lua_error(L);
    }

    block_teleport((block *)lua_touserdata(L, 1), (block *)lua_touserdata(L, 2));
    return 0; /* number of results */
}

int lua_block_swap(lua_State *L)
{
    int n = lua_gettop(L); /* number of arguments */

    if (n != 2 || !lua_isuserdata(L, 1) || !lua_isuserdata(L, 2))
    {
        lua_pushliteral(L, "expected 2 blocks");
        lua_error(L);
    }

    block_swap((block *)lua_touserdata(L, 1), (block *)lua_touserdata(L, 2));
    return 0; /* number of results */
}