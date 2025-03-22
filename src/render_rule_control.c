#include "../include/render_rule_control.h"

static int lua_render_rules_get_size(lua_State *L)
{
    client_render_rules *rules = check_light_userdata(L, 1);

    lua_pushinteger(L, rules->screen_width);
    lua_pushinteger(L, rules->screen_height);

    return 1;
}

static int lua_render_rules_get_order(lua_State *L)
{
    client_render_rules *rules = check_light_userdata(L, 1);

    lua_newtable(L);

    for (int i = 0; i < rules->draw_order.length; i++)
    {
        lua_pushinteger(L, i);
        lua_pushinteger(L, rules->draw_order.data[i]);
        lua_settable(L, -3);
    }

    return 1;
}

static int lua_slice_get(lua_State *L)
{
    client_render_rules *rules = check_light_userdata(L, 1);
    int index = luaL_checkinteger(L, 2);

    if (index > rules->slices.length)
        luaL_error(L, "Index out of range");

    layer_slice slice = rules->slices.data[index];

    lua_newtable(L);
    STRUCT_GET(L, slice, x, lua_pushinteger)
    STRUCT_GET(L, slice, y, lua_pushinteger)
    STRUCT_GET(L, slice, h, lua_pushinteger)
    STRUCT_GET(L, slice, w, lua_pushinteger)
    STRUCT_GET(L, slice, zoom, lua_pushinteger)
    //STRUCT_GET(L, slice, ref, lua_pushlightuserdata)
    NEW_USER_OBJECT(L, Layer, slice.ref);
    lua_setfield(L, -2, "ref");

    return 1;
}

// packs slice from a lua table back into c structure and then sets it in a render rules
static int lua_slice_set(lua_State *L)
{
    client_render_rules *rules = check_light_userdata(L, 1);
    int index = luaL_checkinteger(L, 2);

    if (index > rules->slices.length)
        luaL_error(L, "Index out of range");

    layer_slice slice = {};

    STRUCT_SET(L, slice, x, luaL_checkinteger);
    STRUCT_SET(L, slice, y, luaL_checkinteger);
    STRUCT_SET(L, slice, w, luaL_checkinteger);
    STRUCT_SET(L, slice, h, luaL_checkinteger);
    STRUCT_SET(L, slice, zoom, luaL_checkinteger)
    //STRUCT_SET(L, slice, ref, check_light_userdata)
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper);
    slice.ref = wrapper->l;

    rules->slices.data[index] = slice;

    return 0;
}

void lua_register_render_rules(lua_State *L)
{
    const static luaL_Reg render_rules_lib[] = {
        {"get_size", lua_render_rules_get_size},
        {"get_order", lua_render_rules_get_order},
        {"get_slice", lua_slice_get},
        {"set_slice", lua_slice_set},
        {NULL, NULL}
    };

    luaL_newlib(L, render_rules_lib);
    lua_setglobal(L, "render_rules");
}