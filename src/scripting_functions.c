#include "../include/scripting.h"
#include "../include/level.h"
#include "../include/rendering.h"
#include "../include/block_registry.h"

int lua_register_handler(lua_State *L)
{
    luaL_checkany(L, 1);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    int handler_id = luaL_checkinteger(L, 1);

    scripting_register_event_handler(ref, handler_id);
    return 0;
}

int lua_block_set_id(lua_State *L)
{
    layer *l = check_light_userdata(L, 1);
    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    u64 id = luaL_checknumber(L, 4);

    lua_pushboolean(L, block_set_id(l, x, y, id));

    return 1;
}

int lua_block_get_id(lua_State *L)
{
    layer *l = check_light_userdata(L, 1);
    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    u64 id = 0;

    lua_pushboolean(L, block_get_id(l, x, y, &id));
    lua_pushnumber(L, id);
    return 2;
}

int lua_block_get_vars(lua_State *L)
{
    layer *l = check_light_userdata(L, 1);
    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    blob vars = {};

    lua_pushboolean(L, block_get_vars(l, x, y, &vars));
    lua_pushlightuserdata(L, vars.ptr);
    lua_pushinteger(L, vars.size);

    return 3;
}

int lua_block_set_vars(lua_State *L)
{
    layer *l = check_light_userdata(L, 1);
    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    blob *vars_ref = check_light_userdata(L, 4);

    lua_pushboolean(L, block_set_vars(l, x, y, *vars_ref));

    return 1;
}

int lua_vars_get_integer(lua_State *L)
{
    blob *vars_ref = check_light_userdata(L, 1);
    const char *key = luaL_checkstring(L, 2);
    u64 value = 0;

    lua_pushboolean(L, data_get_num_endianless(*vars_ref, key[0], &value, sizeof(value)));
    lua_pushinteger(L, value);
    return 2;
}

int lua_vars_set_integer(lua_State *L)
{
    blob *vars = check_light_userdata(L, 1);
    const char *key = luaL_checkstring(L, 2);
    u64 value = luaL_checkinteger(L, 3);
    u8 byte_len = luaL_checkinteger(L, 4);

    lua_pushboolean(L, data_set_num_endianless(vars, key[0], &value, byte_len));
    return 1;
}

// rendering things

int lua_render_rules_get_resolutions(lua_State *L)
{
    client_render_rules *rules = check_light_userdata(L, 1);

    lua_pushinteger(L, rules->screen_width);
    lua_pushinteger(L, rules->screen_height);

    return 1;
}

int lua_render_rules_get_order(lua_State *L)
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

int lua_slice_get(lua_State *L)
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
    STRUCT_GET(L, slice, ref, lua_pushlightuserdata)

    return 1;
}

// packs slice from a lua table back into c structure and then sets it in a render rules
int lua_slice_set(lua_State *L)
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
    STRUCT_SET(L, slice, ref, check_light_userdata)

    rules->slices.data[index] = slice;

    return 0;
}

// level managing functions

int lua_create_level(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    int width = luaL_checkinteger(L, 2);
    int height = luaL_checkinteger(L, 3);

    level *l = level_create(name, width, height);

    lua_pushlightuserdata(L, l);

    return 1;
}

int lua_load_registry(lua_State *L)
{
    level *l = (level *)check_light_userdata(L, 1);
    const char *registry_name = luaL_checkstring(L, 2);

    block_registry r = {};

    if (read_block_registry(registry_name, &r) == FAIL)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    (void)vec_push(&l->registries, r);

    lua_pushboolean(L, 1);

    return 1;
}

int lua_create_room(lua_State *L)
{
    level *l = (level *)check_light_userdata(L, 1);
    const char *name = luaL_checkstring(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);

    room_create(l, name, w, h);

    return 0;
}

int lua_get_room(lua_State *L)
{
    level *l = (level *)check_light_userdata(L, 1);
    int index = luaL_checkinteger(L, 2);

    room *r = &l->rooms.data[index];

    lua_pushlightuserdata(L, r);
    return 1;
}

int lua_get_room_count(lua_State *L)
{
    level *l = (level *)check_light_userdata(L, 1);
    lua_pushinteger(L, l->rooms.length);

    return 1;
}

int lua_create_layer(lua_State *L)
{
    level *l = (level *)check_light_userdata(L, 1);
    room *r = (room *)check_light_userdata(L, 2);
    const char *registry_name = luaL_checkstring(L, 3);
    u8 bytes_per_block = luaL_checkinteger(L, 4);
    bool use_vars = lua_toboolean(L, 5);

    block_registry *reg = find_registry(l->registries, (char *)registry_name);

    if (reg == NULL)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    layer_create(r, reg, bytes_per_block, use_vars ? LAYER_FLAG_HAS_VARS : 0);

    lua_pushboolean(L, 1);
    return 1;
}