#include "../include/image_editing.h"
#include "../include/scripting_bindings.h"

// ###############//
// ###############//
// ###############//
//  IMAGE EDITING //
// ###############//
// ###############//
// ###############//

#define PUSHNEWIMAGE(L, i, name)                                                   \
    ImageWrapper *name = (ImageWrapper *)lua_newuserdata(L, sizeof(ImageWrapper)); \
    name->img = (i);                                                               \
    luaL_getmetatable(L, "Image");                                                 \
    lua_setmetatable(L, -2);

typedef struct
{
    image *img;
} ImageWrapper;

static int image_gc(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    if (wrapper->img)
    {
        free_image(wrapper->img);
        wrapper->img = NULL;
    }

    return 0;
}

static int create_image_lua(lua_State *L)
{
    u32 width = luaL_checkinteger(L, 1);
    u32 height = luaL_checkinteger(L, 2);

    PUSHNEWIMAGE(L, create_image(width, height), wrapper);
    return 1;
}

static int image_size_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    lua_pushinteger(L, wrapper->img->width);
    lua_pushinteger(L, wrapper->img->height);

    return 2;
}

static int clear_image_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    clear_image(wrapper->img);

    lua_pushvalue(L, 1);
    return 1;
}

static int adjust_brightness_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    float brightness = luaL_checknumber(L, 2);

    adjust_brightness(wrapper->img, brightness);

    lua_pushvalue(L, 1);
    return 1;
}

static int apply_color_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    u8 color[4] = {
        luaL_checkinteger(L, 2),
        luaL_checkinteger(L, 3),
        luaL_checkinteger(L, 4),
        luaL_checkinteger(L, 5)

    };

    apply_color(wrapper->img, color);

    lua_pushvalue(L, 1);
    return 1;
}

static int gamma_correction_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");
    float gamma = luaL_checknumber(L, 2);

    gamma_correction(wrapper->img, gamma);

    lua_pushvalue(L, 1);

    return 1;
}

static int fill_color_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    u8 color[4] = {
        luaL_checkinteger(L, 2),
        luaL_checkinteger(L, 3),
        luaL_checkinteger(L, 4),
        luaL_checkinteger(L, 5)};

    fill_color(wrapper->img, color);

    lua_pushvalue(L, 1);
    return 1;
}

static int overlay_image_lua(lua_State *L)
{
    ImageWrapper *dst = (ImageWrapper *)luaL_checkudata(L, 1, "Image");
    ImageWrapper *src = (ImageWrapper *)luaL_checkudata(L, 2, "Image");

    u32 x = luaL_checkinteger(L, 3);
    u32 y = luaL_checkinteger(L, 4);

    overlay_image(dst->img, src->img, x, y);

    lua_pushvalue(L, 1);
    return 1;
}

static int rotate_image_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    u8 angle = luaL_checkinteger(L, 2);

    PUSHNEWIMAGE(L, rotate_image(wrapper->img, angle), result);

    return 1;
}

static int flip_image_horizontal_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    PUSHNEWIMAGE(L, flip_image_horizontal(wrapper->img), result);

    return 1;
}

static int flip_image_vertical_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    PUSHNEWIMAGE(L, flip_image_vertical(wrapper->img), result);

    return 1;
}

static int copy_image_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    PUSHNEWIMAGE(L, copy_image(wrapper->img), result);

    return 1;
}

static int save_image_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    const char *path = luaL_checkstring(L, 2);
    save_image(wrapper->img, path);

    return 0;
}

static int load_image_lua(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);

    PUSHNEWIMAGE(L, load_image(path), result);
    return 1;
}

// geometry

static int crop_image_lua(lua_State *L)
{
    ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

    u32 x = luaL_checkinteger(L, 2);
    u32 y = luaL_checkinteger(L, 3);
    u32 width = luaL_checkinteger(L, 4);
    u32 height = luaL_checkinteger(L, 5);

    PUSHNEWIMAGE(L, crop_image(wrapper->img, x, y, width, height), result);
    return 1;
}

void load_image_editing_library(lua_State *L)
{
    const static luaL_Reg image_methods[] = {
        {"crop", crop_image_lua},
        {"rotate", rotate_image_lua},
        {"flip_horizontal", flip_image_horizontal_lua},
        {"flip_vertical", flip_image_vertical_lua},

        {"add_brightness", adjust_brightness_lua},
        {"add_color", apply_color_lua},
        {"gamma_correction", gamma_correction_lua},

        //{"blend", blend_images_lua},
        {"size", image_size_lua},
        {"overlay", overlay_image_lua},

        {"clear", clear_image_lua},
        {"fill", fill_color_lua},
        {"copy", copy_image_lua},

        {"save", save_image_lua},

        {NULL, NULL}

    };

    const static luaL_Reg image_editing_lib[] = {
        {"create", create_image_lua},
        {"load", load_image_lua},
        {NULL, NULL}

    };

    // Create metatable for Image userdata
    luaL_newmetatable(L, "Image");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, image_methods, 0);
    lua_pushcfunction(L, image_gc);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1);

    // Create library
    luaL_newlib(L, image_editing_lib);
    lua_setglobal(L, "ie");
}

// ####################//
// ####################//
// ####################//
//  RENDER RULE ACCESS //
// ####################//
// ####################//
// ####################//

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

    LUA_EXPECT_UNSIGNED(L);
    STRUCT_SET(L, slice, x, LUA_TNUMBER, lua_tointeger);
    LUA_EXPECT_UNSIGNED(L);
    STRUCT_SET(L, slice, y, LUA_TNUMBER, lua_tointeger);
    LUA_EXPECT_UNSIGNED(L);
    STRUCT_SET(L, slice, h, LUA_TNUMBER, lua_tointeger);
    LUA_EXPECT_UNSIGNED(L);
    STRUCT_SET(L, slice, w, LUA_TNUMBER, lua_tointeger);
    LUA_EXPECT_UNSIGNED(L);
    STRUCT_SET(L, slice, zoom, LUA_TNUMBER, lua_tointeger);
    // STRUCT_SET(L, slice, ref, LUA_TUSERDATA, lua_touserdata);
    if (lua_getfield(L, -1, "ref") == 7)
    {
        LuaHolder *wrapper = (LuaHolder *)luaL_checkudata(L, -1, "Layer");
        slice.ref = wrapper->l;
        // LOG_DEBUG("putting %p as a ref for a slice", slice.ref);
        lua_pop(L, 1);
    }
    else
        luaL_error(L, "STRUCT_SET: expected a "
                      "LUA_TUSERDATA "
                      "for a field "
                      "ref");

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

// #####################//
// #####################//
// #####################//
//  LEVE EDITING        //
// #####################//
// #####################//
// #####################//

// level-related stuff

static int lua_uuid_universal(lua_State *L)
{
    void *ptr = 0;
    ptr = luaL_testudata(L, 1, "Level");
    if (ptr != 0)
    {
        lua_pushinteger(L, ((level *)ptr)->uuid);
        return 1;
    }
    ptr = luaL_testudata(L, 1, "Room");
    if (ptr != 0)
    {
        lua_pushinteger(L, ((room *)ptr)->uuid);
        return 1;
    }
    ptr = luaL_testudata(L, 1, "Layer");
    if (ptr)
    {
        lua_pushinteger(L, ((layer *)ptr)->uuid);
        return 1;
    }
    ptr = luaL_testudata(L, 1, "BlockRegistry");
    if (ptr)
    {
        lua_pushinteger(L, ((block_registry *)ptr)->uuid);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int lua_sound_play(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Sound, wrapper, 1);

    lua_pushinteger(L, Mix_PlayChannel(-1, wrapper->s->obj, 0));

    return 1;
}

static int lua_sound_play_randomly(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Sound, wrapper, 1);

    lua_pushinteger(L, Mix_PlayChannel(-1, wrapper->s->obj, 0));

    return 1;
}

static int lua_registry_get_name(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, BlockRegistry, wrapper, 1);

    lua_pushstring(L, wrapper->reg->name);
    return 1;
}

static int lua_cast_to_table(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, BlockRegistry, wrapper, 1);

    lua_newtable(L);

    block_resources r;
    i32 i;
    vec_foreach(&wrapper->reg->resources, r, i)
    {
        lua_newtable(L);

        STRUCT_GET(L, r, id, lua_pushinteger);

        STRUCT_GET(L, r, lua_script_filename, lua_pushstring);

        STRUCT_GET(L, r, anim_controller, lua_pushinteger);
        STRUCT_GET(L, r, type_controller, lua_pushinteger);
        STRUCT_GET(L, r, flip_controller, lua_pushinteger);
        STRUCT_GET(L, r, rotation_controller, lua_pushinteger);
        STRUCT_GET(L, r, override_frame, lua_pushinteger);

        STRUCT_GET(L, r, frames_per_second, lua_pushinteger);
        STRUCT_GET(L, r, flags, lua_pushinteger);

        if (r.all_fields != NULL)
        {
            lua_newtable(L);
            HASHTABLE_FOREACH_EXEC(r.all_fields, node, {
                lua_pushstring(L, node->value.str);
                lua_setfield(L, -2, node->key.str);
            })
            lua_setfield(L, -2, "all_fields");
        }

        texture t = r.block_texture;

        lua_newtable(L);
        {
            STRUCT_GET(L, t, filename, lua_pushstring);
            STRUCT_GET(L, t, gl_id, lua_pushinteger);
            STRUCT_GET(L, t, width, lua_pushinteger);
            STRUCT_GET(L, t, height, lua_pushinteger);
            STRUCT_GET(L, t, frames, lua_pushinteger);
            STRUCT_GET(L, t, types, lua_pushinteger);
            STRUCT_GET(L, t, total_frames, lua_pushinteger);
        }
        lua_setfield(L, -2, "texture");

        if (r.sounds.length > 0)
        {
            lua_newtable(L);
            {
                sound s;
                u32 j;
                vec_foreach(&r.sounds, s, j)
                {
                    lua_newtable(L);
                    STRUCT_GET(L, s, filename, lua_pushstring);
                    STRUCT_GET(L, s, length_ms, lua_pushinteger);

                    NEW_USER_OBJECT(L, Sound, &r.sounds.data[j]);

                    lua_seti(L, -2, j + 1);
                }
            }
            lua_setfield(L, -2, "sounds");
        }

        lua_seti(L, -2, i + 1);
    }

    return 1;
}

static int lua_level_create(lua_State *L)
{
    NEW_USER_OBJECT(L, Level, level_create(luaL_checkstring(L, 1)));
    return 1;
}

static int lua_level_load_registry(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
    const char *registry_name = luaL_checkstring(L, 2);

    block_registry *r = calloc(1, sizeof(block_registry));

    int success = read_block_registry(registry_name, r) == SUCCESS;
    lua_pushboolean(L, success);

    if (success)
    {
        (void)vec_push(&wrapper->lvl->registries, r);

        scripting_load_scripts(r);
        NEW_USER_OBJECT(L, BlockRegistry, r);
    }
    else
    {
        free(r);
    }

    return success ? 2 : 1;
}

static int lua_level_gc(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);

    if (FLAG_GET(wrapper->lvl->flags, SHARED_FLAG_GC_AWARE))
        return 0;

    free_level(wrapper->lvl);

    free(wrapper->lvl);

    return 0;
}

static int lua_level_get_name(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
    lua_pushstring(L, wrapper->lvl->name);
    return 1;
}

static int lua_level_get_room(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
    int index = luaL_checkinteger(L, 2);

    if (index >= wrapper->lvl->rooms.length)
        luaL_error(L, "Index out of range");

    room *r = wrapper->lvl->rooms.data[index];

    NEW_USER_OBJECT(L, Room, r);
    // FLAG_SET(__Room->r->flags, SHARED_FLAG_GC_AWARE, 1); // mark it as gc aware, so it doesn't get freed

    return 1;
}

static int lua_level_get_room_count(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
    lua_pushinteger(L, wrapper->lvl->rooms.length);
    return 1;
}

static int lua_level_get_room_by_name(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
    const char *name = luaL_checkstring(L, 2);

    for (int i = 0; i < wrapper->lvl->rooms.length; i++)
    {
        if (strcmp(((room *)wrapper->lvl->rooms.data[i])->name, name) == 0)
        {
            NEW_USER_OBJECT(L, Room, wrapper->lvl->rooms.data[i]);
            // FLAG_SET(__Room->r->flags, SHARED_FLAG_GC_AWARE, 1); // mark it as gc aware, so it doesn't get freed
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int lua_level_new_room(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
    const char *name = luaL_checkstring(L, 2);
    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);

    NEW_USER_OBJECT(L, Room, room_create(wrapper->lvl, name, x, y));
    // FLAG_SET(__Room->r->flags, SHARED_FLAG_GC_AWARE, 1); // mark it as gc aware, so it doesn't get freed
    return 1;
}

static int lua_room_get_name(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
    lua_pushstring(L, wrapper->r->name);
    return 1;
}

static int lua_room_get_size(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
    lua_pushinteger(L, wrapper->r->width);
    lua_pushinteger(L, wrapper->r->height);
    return 2;
}

static int lua_room_new_layer(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
    const char *registry_name = luaL_checkstring(L, 2);

    int bytes_per_block = luaL_checkinteger(L, 3);
    int byte_per_index = luaL_checkinteger(L, 4);
    int flags = luaL_checkinteger(L, 5);

    block_registry *reg = find_registry((((level *)wrapper->r->parent_level)->registries), (char *)registry_name);

    if (!reg)
        luaL_error(L, "Registry %s not found", registry_name);

    NEW_USER_OBJECT(L, Layer, layer_create(wrapper->r, reg, bytes_per_block, byte_per_index, flags));

    return 1;
}

static int lua_room_get_layer(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
    int index = luaL_checkinteger(L, 2);

    if (index >= wrapper->r->layers.length)
        luaL_error(L, "Index out of range");

    NEW_USER_OBJECT(L, Layer, wrapper->r->layers.data[index]);
    // FLAG_SET(__Layer->l->flags, SHARED_FLAG_GC_AWARE, 1); // mark it as gc aware, so it doesn't get freed
    return 1;
}

static int lua_room_get_layer_count(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
    lua_pushinteger(L, wrapper->r->layers.length);
    return 1;
}

// layer-related functions

// pastes the block from the registry into the layer
// also triggers a block create event

static int lua_layer_paste_block(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    u64 id = luaL_checknumber(L, 4);

    if (!wrapper->l->registry)
        luaL_error(L, "Layer has no registry");

    if (id >= wrapper->l->registry->resources.length)
        luaL_error(L, "Block ID out of range");

    block_resources *res = &wrapper->l->registry->resources.data[id];

    u64 old_id = 0;
    u8 status = block_get_id(wrapper->l, x, y, &old_id) == SUCCESS;

    status = block_set_id(wrapper->l, x, y, id) == SUCCESS;
    status &= block_set_vars(wrapper->l, x, y, res->vars) == SUCCESS;

    block_update_event e = {
        .type = ENGINE_BLOCK_CREATE,
        .x = x,
        .y = y,
        .previous_id = old_id,
        .new_id = id,
        .layer_ptr = wrapper->l,
        .room_ptr = wrapper->l->parent_room,
    };

    SDL_PushEvent((SDL_Event *)&e);

    lua_pushboolean(L, status);
    return 1;
}

static int lua_layer_move_block(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    u32 delta_x = luaL_checknumber(L, 4);
    u32 delta_y = luaL_checknumber(L, 5);

    lua_pushboolean(L, block_move(wrapper->l, x, y, delta_x, delta_y) == SUCCESS);

    return 1;
}

static int lua_layer_set_id(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    u64 id = luaL_checknumber(L, 4);

    lua_pushboolean(L, block_set_id(wrapper->l, x, y, id) == SUCCESS);

    return 1;
}

static int lua_block_get_id(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    u64 id = 0;

    lua_pushboolean(L, block_get_id(wrapper->l, x, y, &id) == SUCCESS);
    lua_pushnumber(L, id);

    return 2;
}

static int lua_block_get_vars(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    blob *vars = NULL;

    lua_pushboolean(L, block_get_vars(wrapper->l, x, y, &vars) == SUCCESS);
    NEW_USER_OBJECT(L, Vars, vars);

    return 2;
}

static int lua_block_set_vars(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

    u32 x = luaL_checknumber(L, 2);
    u32 y = luaL_checknumber(L, 3);
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper_vars, 4);

    lua_pushboolean(L, block_set_vars(wrapper->l, x, y, *wrapper_vars->b) == SUCCESS);

    return 1;
}

static int lua_bprintf(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);
    u64 character_id = luaL_checkinteger(L, 2);
    u32 orig_x = luaL_checkinteger(L, 3);
    u32 orig_y = luaL_checkinteger(L, 4);
    u32 limit = luaL_checkinteger(L, 5);
    const char *format = luaL_checkstring(L, 6);
    bprintf(wrapper->l, character_id, orig_x, orig_y, limit, format);
    return 0;
}

// vars

static int lua_vars_length(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);
    lua_pushinteger(L, wrapper->b->length);
    return 1;
}

static int lua_vars_get_var(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);
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
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);
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
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);
    const char *key = luaL_checkstring(L, 2);
    const char *value = luaL_checkstring(L, 3);

    if (strlen(key) > 1)
        luaL_error(L, "Key must be a single character");

    lua_pushboolean(L, var_set_str(wrapper->b, key[0], value) == SUCCESS);
    return 1;
}

static int lua_vars_set_number(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);
    const char *key = luaL_checkstring(L, 2);
    lua_Number number = luaL_checknumber(L, 3);
    u8 bytes = luaL_checkinteger(L, 4);
    u8 is_signed = luaL_checkinteger(L, 5);

    if (strlen(key) > 1)
        luaL_error(L, "Key must be a single character");

    u8 status = 0;
    if (is_signed)
        switch (bytes)
        {
        case 1:
            status = var_set_i8(wrapper->b, key[0], (i8)number) == SUCCESS;
            break;
        case 2:
            status = var_set_i16(wrapper->b, key[0], (i16)number) == SUCCESS;
            break;
        case 3:
            status = var_set_i32(wrapper->b, key[0], (i32)number) == SUCCESS;
            break;
        case 4:
            status = var_set_i64(wrapper->b, key[0], (i64)number) == SUCCESS;
            break;
        default:
            break;
        }
    else
        switch (bytes)
        {
        case 1:
            status = var_set_u8(wrapper->b, key[0], (u8)number) == SUCCESS;
            break;
        case 2:
            status = var_set_u16(wrapper->b, key[0], (u16)number) == SUCCESS;
            break;
        case 3:
            status = var_set_u32(wrapper->b, key[0], (u32)number) == SUCCESS;
            break;
        case 4:
            status = var_set_u64(wrapper->b, key[0], (u64)number) == SUCCESS;
            break;
        default:
            break;
        }

    lua_pushboolean(L, status);

    return 1;
}

static int lua_vars_get_size(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);
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

static int lua_vars_tostring(lua_State *L)
{
    LUA_CHECK_USER_OBJECT(L, Vars, wrapper, 1);
    char buffer[512];
    dbg_data_layout(*wrapper->b, buffer);
    lua_pushstring(L, buffer);
    return 1;
}

void lua_sound_register(lua_State *L)
{
    const static luaL_Reg sound_methods[] = {
        {"play", lua_sound_play},
        {NULL, NULL},
    };

    luaL_newmetatable(L, "Sound");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, sound_methods, 0);
}

void lua_block_registry_register(lua_State *L)
{
    const static luaL_Reg registry_methods[] = {
        {"get_name", lua_registry_get_name},
        {"to_table", lua_cast_to_table},
        {"uuid", lua_uuid_universal},
        {NULL, NULL},
    };

    luaL_newmetatable(L, "BlockRegistry");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, registry_methods, 0);
}

void lua_level_register(lua_State *L)
{
    const static luaL_Reg level_methods[] = {
        {"__gc", lua_level_gc},
        {"load_registry", lua_level_load_registry},
        {"get_name", lua_level_get_name},
        {"get_room", lua_level_get_room},
        {"get_room_count", lua_level_get_room_count},
        {"find_room", lua_level_get_room_by_name},
        {"new_room", lua_level_new_room},
        {"uuid", lua_uuid_universal},
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
        {"uuid", lua_uuid_universal},
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
        {"paste_block", lua_layer_paste_block},
        {"set_id", lua_layer_set_id},
        {"get_id", lua_block_get_id},
        {"move_block", lua_layer_move_block},
        {"get_vars", lua_block_get_vars},
        {"set_vars", lua_block_set_vars},
        {"bprint", lua_bprintf},
        {"uuid", lua_uuid_universal},
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
        {"set_number", lua_vars_set_number},
        {"get_var", lua_vars_get_var},
        {"get_size", lua_vars_get_size},
        {"__tostring", lua_vars_tostring},
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

void lua_register_engine_objects(lua_State *L)
{
    lua_level_register(g_L); /* level editing */
    lua_room_register(g_L);
    lua_layer_register(g_L);
    lua_vars_register(g_L);
    lua_block_registry_register(g_L);
    lua_sound_register(g_L);
    load_image_editing_library(g_L); /* image editing */
}