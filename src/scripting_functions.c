#include "../include/scripting.h"
#include "../include/level.h"
#include "../include/rendering.h"

int lua_set_block_id(lua_State *L)
{
    scripting_check_arguments(L, 4, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER);

    layer *l = (layer *)lua_touserdata(L, 1);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);
    int id = lua_tonumber(L, 7);

    int ret = block_set_id(l, x, y, id);

    lua_pushboolean(L, ret);
    return 1;
}

int lua_get_block_id(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER);

    layer *l = (layer *)lua_touserdata(L, 1);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);

    u64 id = 0;
    int ret = block_get_id(l, x, y, &id);

    lua_pushboolean(L, ret);
    if (ret == FAIL)
        return 1;

    lua_pushnumber(L, id);

    return 2;
}

int lua_get_block_vars(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TNUMBER);
    layer *l = (layer *)lua_touserdata(L, 1);
    int x = lua_tonumber(L, 3);
    int y = lua_tonumber(L, 4);

    blob vars;

    int ret = block_get_vars(l, x, y, &vars);

    lua_pushboolean(L, ret);
    if (ret == FAIL)
        return 1;

    lua_pushlightuserdata(L, vars.ptr);
    lua_pushinteger(L, vars.size);

    return 3;
}

int lua_vars_get_integer(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TSTRING);
    blob vars = *(blob *)lua_touserdata(L, 1);
    const char *key = lua_tostring(L, 2);
    u64 value = 0;

    int ret = data_get_num_endianless(vars, key[0], &value, sizeof(value));

    lua_pushboolean(L, ret);
    if (ret == FAIL)
        return 1;
    lua_pushinteger(L, value);
    return 2;
}

int lua_vars_set_integer(lua_State *L)
{
    scripting_check_arguments(L, 4, LUA_TLIGHTUSERDATA, LUA_TSTRING, LUA_TNUMBER, LUA_TNUMBER);
    blob *vars = (blob *)lua_touserdata(L, 1);
    const char *key = lua_tostring(L, 2);
    u64 value = lua_tointeger(L, 3);
    u8 byte_len = lua_tointeger(L, 4);

    int ret = data_set_num_endianless(vars, key[0], &value, byte_len);

    lua_pushboolean(L, ret);
    return 1;
}

// rendering things

int lua_render_rules_get_resolutions(lua_State *L)
{
    scripting_check_arguments(L, 1, LUA_TLIGHTUSERDATA);

    client_render_rules *rules = lua_touserdata(L, 1);

    lua_pushinteger(L, rules->screen_width);
    lua_pushinteger(L, rules->screen_height);

    return 1; /* number of results */
}

int lua_render_rules_get_order(lua_State *L)
{
    scripting_check_arguments(L, 1, LUA_TLIGHTUSERDATA);

    client_render_rules *rules = lua_touserdata(L, 1);

    lua_newtable(L);

    for (int i = 0; i < rules->draw_order.length; i++)
    {
        lua_pushinteger(L, i);
        lua_pushinteger(L, rules->draw_order.data[i]);
        lua_settable(L, -3);
    }

    return 1; /* number of results */
}

// unpacks data from a slice object
int lua_slice_get(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TNUMBER);

    client_render_rules *rules = lua_touserdata(L, 1);
    int index = lua_tointeger(L, 2);

    STRUCT_CHECK(L, rules->slices.length, index)

    layer_slice slice = rules->slices.data[index];

    lua_newtable(L);
    STRUCT_GETFIELD(L, slice, x, lua_pushinteger)
    STRUCT_GETFIELD(L, slice, y, lua_pushinteger)
    STRUCT_GETFIELD(L, slice, h, lua_pushinteger)
    STRUCT_GETFIELD(L, slice, w, lua_pushinteger)
    STRUCT_GETFIELD(L, slice, zoom, lua_pushinteger)
    STRUCT_GETFIELD(L, slice, ref, lua_pushlightuserdata)

    return 1;
}

// packs slice from a lua table back into c structure and then sets it in a render rules
int lua_slice_set(lua_State *L)
{
    scripting_check_arguments(L, 3, LUA_TLIGHTUSERDATA, LUA_TNUMBER, LUA_TTABLE);

    client_render_rules *rules = lua_touserdata(L, 1);
    int index = lua_tointeger(L, 2);
    layer_slice slice = {};

    STRUCT_SETFIELD(L, slice, x, LUA_TNUMBER, lua_tointeger);
    STRUCT_SETFIELD(L, slice, y, LUA_TNUMBER, lua_tointeger);
    STRUCT_SETFIELD(L, slice, w, LUA_TNUMBER, lua_tointeger);
    STRUCT_SETFIELD(L, slice, h, LUA_TNUMBER, lua_tointeger);
    STRUCT_SETFIELD(L, slice, zoom, LUA_TNUMBER, lua_tointeger)
    STRUCT_SETFIELD(L, slice, ref, LUA_TLIGHTUSERDATA, lua_touserdata)

    STRUCT_CHECK(L, rules->slices.length, index)

    rules->slices.data[index] = slice;

    return 0;
}
