#include "../include/scripting.h"
#include "../include/image_editing.h"
#include "../include/scripting_bindings.h"

lua_State *g_L = 0;

vec_int_t handlers[32] = {};

static int lua_register_handler(lua_State *L)
{
    luaL_checkany(L, 1);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    int handler_id = luaL_checkinteger(L, 1);

    scripting_register_event_handler(ref, handler_id);
    return 0;
}

void scripting_register(lua_State *L)
{
    const static luaL_Reg blockengine_lib[] = {

        {"register_handler", lua_register_handler},

        {NULL, NULL}

    };

    luaL_newlib(L, blockengine_lib); // creates a table with blockengine functions
    lua_setglobal(L, "blockengine"); // sets the table as global variable "blockengine"
                                     // 0 objects on stack now
}

void *check_light_userdata(lua_State *L, int index)
{
    return lua_islightuserdata(L, index) ? lua_touserdata(L, index) : (void *)0 + luaL_error(L, "Expected light userdata at index %d", index);
}

void scripting_init()
{
    g_L = luaL_newstate(); /* opens Lua */
    luaL_openlibs(g_L);
    scripting_register(g_L); /* register all blockengine functions */

    lua_register_engine_objects(g_L);

    lua_logging_register(g_L);
    lua_level_editing_lib_register(g_L);
    lua_register_render_rules(g_L); /* client render rules */
}

void scripting_close()
{
    lua_close(g_L);
}

u16 get_lookup_id(u32 type)
{
    if (type >= SDL_QUIT && type < SDL_DISPLAYEVENT) // quit events
        return 0;
    if (type >= SDL_DISPLAYEVENT && type < SDL_WINDOWEVENT) // display events
        return 1;
    if (type >= SDL_WINDOWEVENT && type < SDL_KEYDOWN) // window events
        return 2;
    if (type >= SDL_KEYDOWN && type < SDL_MOUSEMOTION) // key events
        return 3;
    if (type >= SDL_MOUSEMOTION && type < SDL_MOUSEWHEEL) // mouse events
        return 4;
    if (type >= SDL_MOUSEWHEEL && type < SDL_JOYAXISMOTION) // mouse events
        return 5;
    if (type >= SDL_JOYAXISMOTION && type < SDL_CONTROLLERAXISMOTION) // joystick events
        return 6;
    if (type >= SDL_CONTROLLERAXISMOTION && type < SDL_FINGERDOWN) // controller events
        return 7;
    if (type >= SDL_FINGERDOWN && type < SDL_DOLLARGESTURE) // touch events
        return 8;
    if (type >= SDL_DOLLARGESTURE && type < SDL_CLIPBOARDUPDATE) // gesture events
        return 9;
    if (type >= SDL_CLIPBOARDUPDATE && type < SDL_DROPFILE) // clipboard events
        return 10;
    if (type >= SDL_DROPFILE && type < SDL_AUDIODEVICEADDED) // drop events
        return 11;
    if (type >= SDL_AUDIODEVICEADDED && type < SDL_SENSORUPDATE) // audio events
        return 12;
    if (type >= SDL_SENSORUPDATE && type < SDL_RENDER_TARGETS_RESET) // sensor events
        return 13;
    if (type >= SDL_RENDER_TARGETS_RESET && type < SDL_POLLSENTINEL) // render events
        return 14;
    if (type >= SDL_POLLSENTINEL && type < SDL_USEREVENT) // poll events
        return 15;
    if (type >= ENGINE_BLOCK_UDPATE && type < ENGINE_BLOCK_SECTION_END) // block events
        return 16;
    if (type >= ENGINE_BLOB_UPDATE && type < ENGINE_BLOB_SECTION_END) // blob events
        return 17;
    if (type == ENGINE_TICK)
        return 18;
    if (type == ENGINE_INIT)
        return 19;
    return sizeof(handlers); // unknown event, must be an error
}

int push_event_args(SDL_Event *e)
{
    if (!e)
        return 0;

    char buf[16] = {};

    switch (e->type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        lua_pushinteger(g_L, e->key.keysym.sym);
        lua_pushinteger(g_L, e->key.keysym.mod);
        lua_pushinteger(g_L, e->key.state);
        lua_pushinteger(g_L, e->key.repeat);
        return 4;
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        lua_pushinteger(g_L, e->button.x);
        lua_pushinteger(g_L, e->button.y);
        // lua_pushinteger(g_L, e->motion.type == SDL_MOUSEBUTTONDOWN ? 1 : 0);
        //  lua_pushinteger(g_L, e->button.button);
        lua_pushinteger(g_L, e->button.state);
        lua_pushinteger(g_L, e->button.clicks);
        return 4;
    case SDL_MOUSEWHEEL:
        lua_pushinteger(g_L, e->wheel.x);
        lua_pushinteger(g_L, e->wheel.y);
        lua_pushinteger(g_L, e->wheel.mouseX);
        lua_pushinteger(g_L, e->wheel.mouseY);
        return 4;
    case ENGINE_BLOCK_UDPATE:
    case ENGINE_BLOCK_ERASED:
    case ENGINE_BLOCK_CREATE:
        block_update_event *block_event = (block_update_event *)e;

        NEW_USER_OBJECT(g_L, Room, block_event->room_ptr);
        NEW_USER_OBJECT(g_L, Layer, block_event->layer_ptr);

        lua_pushinteger(g_L, block_event->new_id);
        lua_pushinteger(g_L, block_event->previous_id);

        lua_pushinteger(g_L, block_event->x);
        lua_pushinteger(g_L, block_event->y);

        return 6;
    case ENGINE_BLOB_UPDATE:
    case ENGINE_BLOB_ERASED:
    case ENGINE_BLOB_CREATE:
        blob_update_event *blob_event = (blob_update_event *)e;

        NEW_USER_OBJECT(g_L, Room, blob_event->room_ptr);
        NEW_USER_OBJECT(g_L, Layer, blob_event->layer_ptr);

        lua_pushinteger(g_L, blob_event->new_id);

        lua_pushinteger(g_L, blob_event->x);
        lua_pushinteger(g_L, blob_event->y);

        lua_pushinteger(g_L, blob_event->pos);

        buf[0] = blob_event->letter;
        lua_pushstring(g_L, buf);
        lua_pushinteger(g_L, blob_event->size);

        lua_pushlightuserdata(g_L, blob_event->ptr);

        return 9;
    case ENGINE_TICK:
        lua_pushinteger(g_L, e->user.code);
        return 1;
    }
    return 0;
}

void call_handlers(SDL_Event e)
{
    u16 lookup_id = get_lookup_id(e.type);

    if (lookup_id == sizeof(handlers))
    {
        LOG_ERROR("Unknown event type %d", e.type);
        return;
    }

    vec_int_t target = handlers[lookup_id];

    const int handler_count = target.length;
    for (int i = 0; i < handler_count; i++)
    {
        lua_geti(g_L, LUA_REGISTRYINDEX, target.data[i]);
        luaL_checktype(g_L, -1, LUA_TFUNCTION);

        u16 args = push_event_args(&e);

        if (lua_pcall(g_L, args, 0, 0) != 0)
        {
            LOG_ERROR("Error calling a handler, id %d: %s", lookup_id, lua_tostring(g_L, -1));
            lua_pop(g_L, 1);
        }
    }
}

void scripting_register_event_handler(int lua_func_ref, int event_type)
{
    u16 lookup_id = get_lookup_id(event_type);

    LOG_DEBUG("registering %d handler", lookup_id);

    if (lookup_id == sizeof(handlers))
    {
        LOG_ERROR("Unknown event type %d", event_type);
        return;
    }

    (void)vec_push(&handlers[lookup_id], lua_func_ref);
}

int scripting_load_file(const char *reg_name, const char *short_filename)
{
    char filename[MAX_PATH_LENGTH] = {};
    snprintf(filename, MAX_PATH_LENGTH, REGISTRIES_FOLDER "/%s/" SCRIPTS_FOLDER "/%s", reg_name, short_filename);
    int status = luaL_dofile(g_L, filename);

    if (status != LUA_OK)
    {
        fprintf(stderr, "Lua error in %s : %s", filename, lua_tostring(g_L, -1));
        return FAIL;
    }

    return SUCCESS;
}

void scripting_load_scripts(block_registry *registry)
{
    block_resources_t *reg = &registry->resources;
    const int length = reg->length;

    const char *reg_name = registry->name;

    for (int i = 0; i < length; i++)
    {
        const char *lua_file = reg->data[i].lua_script_filename;

        if (lua_file)
        {
            LOG_DEBUG("Loading script %s", lua_file);

            lua_pushinteger(g_L, reg->data[i].id);
            lua_setglobal(g_L, "scripting_current_block_id");

            scripting_load_file(reg_name, lua_file);
        }
    }
}
