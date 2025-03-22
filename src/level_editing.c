#include "../include/level_editing.h"

// level-related stuff

static int lua_level_create(lua_State *L)
{
    NEW_USER_OBJECT(L, Level, level_create(luaL_checkstring(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3)));
    return 1;
}

static int lua_level_load_registry(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper);
    const char *registry_name = luaL_checkstring(L, 2);

    block_registry r = {};

    int success = read_block_registry(registry_name, &r) == FAIL;

    if (success)
        (void)vec_push(&wrapper->lvl->registries, r);

    lua_pushboolean(L, success);
    return 1;
}

static int lua_level_gc(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper);

    if (FLAG_GET(wrapper->lvl->flags, SHARED_FLAG_GC_AWARE))
        return 0;

    free_level(wrapper->lvl);
    return 0;
}

static int lua_level_get_name(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper);
    lua_pushstring(L, wrapper->lvl->name);
    return 1;
}

static int lua_level_get_room(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper);
    int index = luaL_checkinteger(L, 2);

    if (index > wrapper->lvl->rooms.length)
        luaL_error(L, "Index out of range");

    room *r = &wrapper->lvl->rooms.data[index];

    NEW_USER_OBJECT(L, Room, r);
    // FLAG_SET(__Room->r->flags, SHARED_FLAG_GC_AWARE, 1); // mark it as gc aware, so it doesn't get freed

    return 1;
}

static int lua_level_get_room_count(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper);
    lua_pushinteger(L, wrapper->lvl->rooms.length);
    return 1;
}

static int lua_level_get_room_by_name(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper);
    const char *name = luaL_checkstring(L, 2);

    for (int i = 0; i < wrapper->lvl->rooms.length; i++)
    {
        if (strcmp(wrapper->lvl->rooms.data[i].name, name) == 0)
        {
            NEW_USER_OBJECT(L, Room, &wrapper->lvl->rooms.data[i]);
            // FLAG_SET(__Room->r->flags, SHARED_FLAG_GC_AWARE, 1); // mark it as gc aware, so it doesn't get freed
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int lua_level_room_create(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper);
    const char *name = luaL_checkstring(L, 2);
    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);

    room_create(wrapper->lvl, name, x, y);

    // vec_push(&wrapper->lvl->rooms, r);
    room_vec_t *rooms = &wrapper->lvl->rooms;
    room *r = &rooms->data[rooms->length - 1];

    NEW_USER_OBJECT(L, Room, r);
    // FLAG_SET(__Room->r->flags, SHARED_FLAG_GC_AWARE, 1); // mark it as gc aware, so it doesn't get freed
    return 1;
}

static int lua_room_get_name(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper);
    lua_pushstring(L, wrapper->r->name);
    return 1;
}

static int lua_room_get_size(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper);
    lua_pushinteger(L, wrapper->r->width);
    lua_pushinteger(L, wrapper->r->height);
    return 2;
}

static int lua_room_new_layer(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper);
    const char *registry_name = luaL_checkstring(L, 2);

    int bytes_per_block = luaL_checkinteger(L, 3);
    int flags = luaL_checkinteger(L, 4);

    block_registry *reg = find_registry(((level *)wrapper->r->parent_level)->registries, (char*)registry_name);

    if (!reg)
        luaL_error(L, "Registry not found");

    layer_create(wrapper->r, reg, bytes_per_block, flags);

    return 0;
}

static int lua_room_get_layer(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper);
    int index = luaL_checkinteger(L, 2);

    if (index > wrapper->r->layers.length)
        luaL_error(L, "Index out of range");

    layer *l = &wrapper->r->layers.data[index];
    NEW_USER_OBJECT(L, Layer, l);
    // FLAG_SET(__Layer->l->flags, SHARED_FLAG_GC_AWARE, 1); // mark it as gc aware, so it doesn't get freed
    return 1;
}

static int lua_room_get_layer_count(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper);
    lua_pushinteger(L, wrapper->r->layers.length);
    return 1;
}

// layer-related functions

static int lua_layer_set_id(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    u64 id = luaL_checknumber(L, 4);

    lua_pushboolean(L, block_set_id(wrapper->l, x, y, id));

    return 1;
}

static int lua_block_get_id(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    u64 id = 0;

    lua_pushboolean(L, block_get_id(wrapper->l, x, y, &id));
    lua_pushnumber(L, id);

    return 2;
}

static int lua_block_get_vars(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    blob vars = {};

    lua_pushboolean(L, block_get_vars(wrapper->l, x, y, &vars));
    NEW_USER_OBJECT(L, Vars, &vars);

    return 1;
}

static int lua_block_set_vars(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper_vars);

    lua_pushboolean(L, block_set_vars(wrapper->l, x, y, *wrapper_vars->b));

    return 1;
}

// vars

static int lua_vars_length(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper);
    lua_pushinteger(L, wrapper->b->length);
    return 1;
}

static int lua_vars_get_var(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper);
    const char *key = luaL_checkstring(L, 2);

    if (strlen(key) > 1)
        luaL_error(L, "Key must be a single character");

    blob b = var_get(*wrapper->b, key[0]);
    if (b.length == 0)
    {
        lua_pushnil(L);
        return 1;
    }

    NEW_USER_OBJECT(L, Vars, &b);
    return 1;
}

static int lua_vars_get_string(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper);
    const char *key = luaL_checkstring(L, 2);

    if (strlen(key) > 1)
        luaL_error(L, "Key must be a single character");

    char *ret = 0;
    u8 status = var_get_str(*wrapper->b, key[0], &ret);

    if (status == SUCCESS)
        lua_pushstring(L, ret);
    else
        lua_pushnil(L);

    return 1;
}

static int lua_vars_set_string(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper);
    const char *key = luaL_checkstring(L, 2);
    const char *value = luaL_checkstring(L, 3);

    if (strlen(key) > 1)
        luaL_error(L, "Key must be a single character");

    lua_pushboolean(L, var_set_str(wrapper->b, key[0], value));
    return 1;
}

static int lua_vars_get_size(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper);
    const char *key = luaL_checkstring(L, 2);
    if (strlen(key) > 1)
        luaL_error(L, "Key must be a single character");

    i16 size = var_size(*wrapper->b, key[0]);
    if (size < 0)
        lua_pushnil(L);
    else
        lua_pushinteger(L, size);

    return 1;
}

void lua_level_register(lua_State *L)
{
    const static luaL_Reg level_methods[] = {
        {"load_registry", lua_level_load_registry},
        {"__gc", lua_level_gc},
        {"get_name", lua_level_get_name},
        {"get_room", lua_level_get_room},
        {"get_room_count", lua_level_get_room_count},
        {"find_room", lua_level_get_room_by_name},
        {"new_room", lua_level_room_create},
        {NULL, NULL},
    };

    luaL_newmetatable(L, "Level");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, level_methods, 0);
}

void lua_room_register(lua_State *L)
{
    const static luaL_Reg room_methods[] = {
        {"get_name", lua_room_get_name},
        {"get_size", lua_room_get_size},
        {"get_layer", lua_room_get_layer},
        {"get_layer_count", lua_room_get_layer_count},
        {"new_layer", lua_room_new_layer},
        {NULL, NULL},
    };

    luaL_newmetatable(L, "Room");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, room_methods, 0);
}

void lua_layer_register(lua_State *L)
{
    const static luaL_Reg layer_methods[] = {
        {"set_id", lua_layer_set_id},
        {"get_id", lua_block_get_id},
        {"get_vars", lua_block_get_vars},
        {"set_vars", lua_block_set_vars},
        {NULL, NULL},
    };

    luaL_newmetatable(L, "Layer");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, layer_methods, 0);
}

void lua_vars_register(lua_State *L)
{
    const static luaL_Reg vars_methods[] = {
        {"get_length", lua_vars_length},
        {"get_string", lua_vars_get_string},
        {"set_string", lua_vars_set_string},
        {"get_var", lua_vars_get_var},
        {"get_size", lua_vars_get_size},
        {NULL, NULL},
    };

    luaL_newmetatable(L, "Vars");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, vars_methods, 0);
}

void lua_level_editing_lib_register(lua_State *L)
{
    const static luaL_Reg level_editing_methods[] = {
        {"create_level", lua_level_create},
        {NULL, NULL},
    };

    luaL_newlib(L, level_editing_methods);
    lua_setglobal(L, "le");
}