#include "include/lua_drawing_functions.h"

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
    STRUCT_GETFIELD(L, slice, mult, lua_pushinteger)

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
    STRUCT_SETFIELD(L, slice, mult, LUA_TNUMBER, lua_tointeger);

    STRUCT_CHECK(L, rules->slices.length, index)

    rules->slices.data[index] = slice;

    return 0;
}