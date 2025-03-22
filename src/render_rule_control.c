#include "../include/render_rule_control.h"

static int lua_render_rules_get_size(lua_State *L)
{
    client_render_rules *rules = check_light_userdata(L, 1);

    lua_pushinteger(L, rules->screen_width);
    lua_pushinteger(L, rules->screen_height);

    return 2;
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

static int lua_render_rules_set_order(lua_State *L)
{
    client_render_rules *rules = check_light_userdata(L, 1);

    vec_deinit(&rules->draw_order);

    // traverse table and set all integers in order

    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        if (lua_isinteger(L, -1))
        {
            int value = lua_tointeger(L, -1);
            (void)vec_push(&rules->draw_order, value);
        }
        else
        {
            int index = lua_tointeger(L, -2);
            luaL_error(L, "Invalid value in table on index %d", index);
        }
        lua_pop(L, 1);
    }
    // Pop table
    lua_pop(L, 1);

    return 0;
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
    // STRUCT_GET(L, slice, ref, lua_pushlightuserdata)
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

    LuaHolder *wrapper = (LuaHolder *)luaL_checkudata(L, 3, "Layer");
    slice.ref = wrapper->l;

    slice.x = luaL_checkinteger(L, 4);
    slice.y = luaL_checkinteger(L, 5);
    slice.w = luaL_checkinteger(L, 6);
    slice.h = luaL_checkinteger(L, 7);
    slice.zoom = luaL_checkinteger(L, 8);

    if (index >= rules->slices.length)
    {
        vec_reserve(&rules->slices, index + 1);
        rules->slices.length = index + 1;
    }
    rules->slices.data[index] = slice;

    return 0;
}

void lua_register_render_rules(lua_State *L)
{
    const static luaL_Reg render_rules_lib[] = {
        {"get_size", lua_render_rules_get_size},
        {"get_order", lua_render_rules_get_order},
        {"set_order", lua_render_rules_set_order},
        {"get_slice", lua_slice_get},
        {"set_slice", lua_slice_set},
        {NULL, NULL}};

    luaL_newlib(L, render_rules_lib);
    lua_setglobal(L, "render_rules");
}