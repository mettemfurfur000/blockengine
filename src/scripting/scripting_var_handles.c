#include "include/scripting_var_handles.h"
#include "include/handle.h"
#include "include/level.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "include/vars.h"
#include "include/vars_utils.h"
#include <string.h>

/* VarHandle userdata stores the owning layer pointer and the packed handle (u32)
   so Lua can keep it and call methods. */

typedef struct VarHandleUser
{
    layer *l;
    u32 packed; /* u32 representation of handle32 (use handle_to_u32/handle_from_u32)
                  stored in a u32 for easy passing to Lua */
} VarHandleUser;

static int lua_varhandle_is_valid(lua_State *L)
{
    VarHandleUser *vh = (VarHandleUser *)luaL_checkudata(L, 1, "VarHandle");
    if (!vh || !vh->l || !vh->l->var_pool.table)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    handle32 h = handle_from_u32(vh->packed);
    lua_pushboolean(L, handle_is_valid(vh->l->var_pool.table, h));
    return 1;
}

static int lua_varhandle_release(lua_State *L)
{
    VarHandleUser *vh = (VarHandleUser *)luaL_checkudata(L, 1, "VarHandle");
    if (!vh || !vh->l || !vh->l->var_pool.table)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    handle32 h = handle_from_u32(vh->packed);
    /* free stored blob and release handle */
    blob *b = (blob *)handle_table_get(vh->l->var_pool.table, h);
    if (b)
    {
        SAFE_FREE(b->ptr);
        SAFE_FREE(b);
    }
    handle_table_release(vh->l->var_pool.table, h);

    lua_pushboolean(L, 1);
    return 1;
}

/* Construct a VarHandle userdata from layer + packed handle (u32) on the stack.
   Used by engine when returning handles to scripts. */
int push_varhandle(lua_State *L, layer *l, u32 packed)
{
    VarHandleUser *vh = (VarHandleUser *)lua_newuserdata(L, sizeof(VarHandleUser));
    vh->l = l;
    vh->packed = packed;
    luaL_getmetatable(L, "VarHandle");
    lua_setmetatable(L, -2);
    return 1;
}

/* Helper: return the blob pointer associated with a VarHandle at stack index `idx`,
   or NULL if invalid. */
blob *get_blob_from_varhandle(lua_State *L, int idx)
{
    if (!luaL_testudata(L, idx, "VarHandle"))
        return NULL;

    VarHandleUser *vh = (VarHandleUser *)luaL_checkudata(L, idx, "VarHandle");
    if (!vh || !vh->l || !vh->l->var_pool.table)
        return NULL;

    handle32 h = handle_from_u32(vh->packed);
    return (blob *)handle_table_get(vh->l->var_pool.table, h);
}

/* File-scope macro to fetch the blob pointer protected by the VarHandle. */
#define VH_GET_BLOB_FROM_L(L, _b_out)                                                                                  \
    handle32 _h = handle_from_u32(((VarHandleUser *)luaL_checkudata(L, 1, "VarHandle"))->packed);                      \
    layer *_l = ((VarHandleUser *)luaL_checkudata(L, 1, "VarHandle"))->l;                                              \
    if (!_l || !_l->var_pool.table)                                                                                    \
        return luaL_error(L, "Invalid VarHandle (no layer/table)");                                                    \
    blob *_b_out = (blob *)handle_table_get(_l->var_pool.table, _h);                                                   \
    if (!_b_out)                                                                                                       \
        return luaL_error(L, "Invalid VarHandle (slot not active)");

/* simple helpers */
static int lua_varhandle_length(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    lua_pushinteger(L, _b->length);
    return 1;
}

static int lua_varhandle_get_string(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *key = luaL_checkstring(L, 2);
    if (strlen(key) > 1)
        return luaL_error(L, "Key must be a single character");
    char *ret = 0;
    u8 status = var_get_str(*_b, key[0], &ret);
    if (status == SUCCESS)
        lua_pushstring(L, ret);
    else
        lua_pushnil(L);
    return 1;
}

static int lua_varhandle_set_string(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *key = luaL_checkstring(L, 2);
    const char *value = luaL_checkstring(L, 3);
    if (strlen(key) > 1)
        return luaL_error(L, "Key must be a single character");
    lua_pushboolean(L, var_set_str(_b, key[0], value) == SUCCESS);
    return 1;
}

static int lua_varhandle_add_variable(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *key = luaL_checkstring(L, 2);
    const int length = luaL_checknumber(L, 3);
    if (strlen(key) > 1)
        return luaL_error(L, "Key must be a single character");
    lua_pushboolean(L, var_add(_b, key[0], length) == SUCCESS);
    return 1;
}
static int lua_varhandle_remove(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *key = luaL_checkstring(L, 2);
    if (strlen(key) > 1)
        return luaL_error(L, "Key must be a single character");
    lua_pushboolean(L, var_delete(_b, key[0]) == SUCCESS);
    return 1;
}

static int lua_varhandle_resize(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *key = luaL_checkstring(L, 2);
    const int new_size = luaL_checknumber(L, 3);
    if (strlen(key) > 1)
        return luaL_error(L, "Key must be a single character");
    lua_pushboolean(L, var_resize(_b, key[0], (u8)new_size) == SUCCESS);
    return 1;
}

static int lua_varhandle_rename(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *old_key = luaL_checkstring(L, 2);
    const char *new_key = luaL_checkstring(L, 3);
    if (strlen(old_key) > 1 || strlen(new_key) > 1)
        return luaL_error(L, "Keys must be single characters");
    lua_pushboolean(L, var_rename(_b, old_key[0], new_key[0]) == SUCCESS);
    return 1;
}

static int lua_varhandle_ensure(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *key = luaL_checkstring(L, 2);
    const int needed = luaL_checknumber(L, 3);
    if (strlen(key) > 1)
        return luaL_error(L, "Key must be a single character");
    i32 pos = ensure_tag(_b, key[0], needed);
    if (pos < 0)
        lua_pushnil(L);
    else
        lua_pushinteger(L, pos);
    return 1;
}

static int lua_varhandle_parse(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *input_string = luaL_checkstring(L, 2);
    lua_pushboolean(L, vars_parse(input_string, _b) == SUCCESS);
    return 1;
}

static int lua_varhandle_raw(lua_State *L)
{
    handle32 _h = handle_from_u32(((VarHandleUser *)luaL_checkudata(L, 1, "VarHandle"))->packed);
    // layer *_l = ((VarHandleUser *)luaL_checkudata(L, 1, "VarHandle"))->l;
    lua_pushinteger(L, _h.index);
    lua_pushinteger(L, _h.validation);
    return 2;
}

/* get_size and tostring */
static int lua_varhandle_get_size(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    const char *key = luaL_checkstring(L, 2);
    if (strlen(key) > 1)
        return luaL_error(L, "Key must be a single character");
    i16 size = var_size(*_b, key[0]);
    if (size < 0)
        lua_pushnil(L);
    else
        lua_pushinteger(L, size);
    return 1;
}

static int lua_varhandle_tostring(lua_State *L)
{
    VH_GET_BLOB_FROM_L(L, _b);
    char buffer[512];
    dbg_data_layout(*_b, buffer);
    lua_pushstring(L, buffer);
    return 1;
}

/* typed setters/getters */
#define VH_SETTER_FN(type, cname)                                                                                      \
    static int lua_varhandle_set_##cname(lua_State *L)                                                                 \
    {                                                                                                                  \
        VH_GET_BLOB_FROM_L(L, _b);                                                                                     \
        const char *key = luaL_checkstring(L, 2);                                                                      \
        lua_Number number = luaL_checknumber(L, 3);                                                                    \
        if (strlen(key) > 1)                                                                                           \
            return luaL_error(L, "Key must be a single character");                                                    \
        lua_pushboolean(L, var_set_##cname(_b, key[0], (type)number) == SUCCESS);                                      \
        return 1;                                                                                                      \
    }

#define VH_GETTER_FN(type, cname)                                                                                      \
    static int lua_varhandle_get_##cname(lua_State *L)                                                                 \
    {                                                                                                                  \
        VH_GET_BLOB_FROM_L(L, _b);                                                                                     \
        const char *key = luaL_checkstring(L, 2);                                                                      \
        if (strlen(key) > 1)                                                                                           \
            return luaL_error(L, "Key must be a single character");                                                    \
        type ret = 0;                                                                                                  \
        if (var_get_##cname(*_b, key[0], &ret) == SUCCESS)                                                             \
            lua_pushinteger(L, ret);                                                                                   \
        else                                                                                                           \
            lua_pushnil(L);                                                                                            \
        return 1;                                                                                                      \
    }

VH_SETTER_FN(i8, i8)
VH_SETTER_FN(i16, i16)
VH_SETTER_FN(i32, i32)
VH_SETTER_FN(i64, i64)

VH_SETTER_FN(u8, u8)
VH_SETTER_FN(u16, u16)
VH_SETTER_FN(u32, u32)
VH_SETTER_FN(u64, u64)

VH_GETTER_FN(i8, i8)
VH_GETTER_FN(i16, i16)
VH_GETTER_FN(i32, i32)
VH_GETTER_FN(i64, i64)

VH_GETTER_FN(u8, u8)
VH_GETTER_FN(u16, u16)
VH_GETTER_FN(u32, u32)
VH_GETTER_FN(u64, u64)

void lua_varhandle_register(lua_State *L)
{
    const static luaL_Reg varhandle_methods[] = {
        {  "is_valid",     lua_varhandle_is_valid},
        {   "release",      lua_varhandle_release},

        {"get_length",       lua_varhandle_length},
        {"get_string",   lua_varhandle_get_string},
        {"set_string",   lua_varhandle_set_string},
        {       "add", lua_varhandle_add_variable},
        {     "parse",        lua_varhandle_parse},

        {    "set_i8",       lua_varhandle_set_i8},
        {   "set_i16",      lua_varhandle_set_i16},
        {   "set_i32",      lua_varhandle_set_i32},
        {   "set_i64",      lua_varhandle_set_i64},
        {    "set_u8",       lua_varhandle_set_u8},
        {   "set_u16",      lua_varhandle_set_u16},
        {   "set_u32",      lua_varhandle_set_u32},
        {   "set_u64",      lua_varhandle_set_u64},

        {    "get_i8",       lua_varhandle_get_i8},
        {   "get_i16",      lua_varhandle_get_i16},
        {   "get_i32",      lua_varhandle_get_i32},
        {   "get_i64",      lua_varhandle_get_i64},
        {    "get_u8",       lua_varhandle_get_u8},
        {   "get_u16",      lua_varhandle_get_u16},
        {   "get_u32",      lua_varhandle_get_u32},
        {   "get_u64",      lua_varhandle_get_u64},

        {  "get_size",     lua_varhandle_get_size},
        {"__tostring",     lua_varhandle_tostring},
        {   "get_raw",          lua_varhandle_raw},
        {    "remove",       lua_varhandle_remove},
        {    "resize",       lua_varhandle_resize},
        {    "rename",       lua_varhandle_rename},
        {    "ensure",       lua_varhandle_ensure},

        {        NULL,                       NULL},
    };

    luaL_newmetatable(L, "VarHandle");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, varhandle_methods, 0);
}
