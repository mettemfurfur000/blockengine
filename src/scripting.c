#include "../include/scripting.h"
// #include "../include/image_editing.h"
#include "../include/events.h"
#include "../include/scripting_bindings.h"

lua_State *g_L = 0;

vec_int_t handlers[128] = {};

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
        //{"register_input", lua_register_block_input},

        {              NULL,                 NULL}
    };

    luaL_newlib(L,
                blockengine_lib);    // creates a table with blockengine functions
    lua_setglobal(L, "blockengine"); // sets the table as global variable
                                     // "blockengine" 0 objects on stack now
}

void *check_light_userdata(lua_State *L, int index)
{
    return lua_islightuserdata(L, index) ? lua_touserdata(L, index)
                                         : (void *)0 + luaL_error(L, "Expected light userdata at index %d", index);
}

void scripting_init()
{
    /* opens Lua */
    g_L = luaL_newstate();
    luaL_openlibs(g_L);
    /* register all blockengine functions */
    scripting_register(g_L);

    lua_register_engine_objects(g_L);

    /* client render rules */
    lua_logging_register(g_L);
    lua_level_editing_lib_register(g_L);
    lua_register_render_rules(g_L);

    /* push an event enum */

    enum_entry entries[] = {
        {  "ENGINE_BLOCK_UDPATE",   ENGINE_BLOCK_UDPATE},
        {  "ENGINE_BLOCK_ERASED",   ENGINE_BLOCK_ERASED},
        {  "ENGINE_BLOCK_CREATE",   ENGINE_BLOCK_CREATE},

        {   "ENGINE_BLOB_UPDATE",    ENGINE_BLOB_UPDATE},
        {   "ENGINE_BLOB_ERASED",    ENGINE_BLOB_ERASED},
        {   "ENGINE_BLOB_CREATE",    ENGINE_BLOB_CREATE},

        {"ENGINE_SPECIAL_SIGNAL", ENGINE_SPECIAL_SIGNAL},

        {          "ENGINE_TICK",           ENGINE_TICK},
        {          "ENGINE_INIT",           ENGINE_INIT},

        {                   NULL,                     0},
    };

    // TODO: sync it wit de script

    scripting_set_global_enum(g_L, entries, "engine_events");

    enum_entry sdl_events[] = {
        {                                "SDL_FIRSTEVENT",                                 SDL_FIRSTEVENT},
        {                                      "SDL_QUIT",                                       SDL_QUIT},
        {                           "SDL_APP_TERMINATING",                            SDL_APP_TERMINATING},
        {                             "SDL_APP_LOWMEMORY",                              SDL_APP_LOWMEMORY},
        {                   "SDL_APP_WILLENTERBACKGROUND",                    SDL_APP_WILLENTERBACKGROUND},
        {                    "SDL_APP_DIDENTERBACKGROUND",                     SDL_APP_DIDENTERBACKGROUND},
        {                   "SDL_APP_WILLENTERFOREGROUND",                    SDL_APP_WILLENTERFOREGROUND},
        {                    "SDL_APP_DIDENTERFOREGROUND",                     SDL_APP_DIDENTERFOREGROUND},
        {                             "SDL_LOCALECHANGED",                              SDL_LOCALECHANGED},
        {                              "SDL_DISPLAYEVENT",                               SDL_DISPLAYEVENT},
        {                               "SDL_WINDOWEVENT",                                SDL_WINDOWEVENT},
        {                                "SDL_SYSWMEVENT",                                 SDL_SYSWMEVENT},
        {                                   "SDL_KEYDOWN",                                    SDL_KEYDOWN},
        {                                     "SDL_KEYUP",                                      SDL_KEYUP},
        {                               "SDL_TEXTEDITING",                                SDL_TEXTEDITING},
        {                                 "SDL_TEXTINPUT",                                  SDL_TEXTINPUT},
        {                             "SDL_KEYMAPCHANGED",                              SDL_KEYMAPCHANGED},
        {                           "SDL_TEXTEDITING_EXT",                            SDL_TEXTEDITING_EXT},
        {                               "SDL_MOUSEMOTION",                                SDL_MOUSEMOTION},
        {                           "SDL_MOUSEBUTTONDOWN",                            SDL_MOUSEBUTTONDOWN},
        {                             "SDL_MOUSEBUTTONUP",                              SDL_MOUSEBUTTONUP},
        {                                "SDL_MOUSEWHEEL",                                 SDL_MOUSEWHEEL},
        {                             "SDL_JOYAXISMOTION",                              SDL_JOYAXISMOTION},
        {                             "SDL_JOYBALLMOTION",                              SDL_JOYBALLMOTION},
        {                              "SDL_JOYHATMOTION",                               SDL_JOYHATMOTION},
        {                             "SDL_JOYBUTTONDOWN",                              SDL_JOYBUTTONDOWN},
        {                               "SDL_JOYBUTTONUP",                                SDL_JOYBUTTONUP},
        {                            "SDL_JOYDEVICEADDED",                             SDL_JOYDEVICEADDED},
        {                          "SDL_JOYDEVICEREMOVED",                           SDL_JOYDEVICEREMOVED},
        {                         "SDL_JOYBATTERYUPDATED",                          SDL_JOYBATTERYUPDATED},
        {                      "SDL_CONTROLLERAXISMOTION",                       SDL_CONTROLLERAXISMOTION},
        {                      "SDL_CONTROLLERBUTTONDOWN",                       SDL_CONTROLLERBUTTONDOWN},
        {                        "SDL_CONTROLLERBUTTONUP",                         SDL_CONTROLLERBUTTONUP},
        {                     "SDL_CONTROLLERDEVICEADDED",                      SDL_CONTROLLERDEVICEADDED},
        {                   "SDL_CONTROLLERDEVICEREMOVED",                    SDL_CONTROLLERDEVICEREMOVED},
        {                  "SDL_CONTROLLERDEVICEREMAPPED",                   SDL_CONTROLLERDEVICEREMAPPED},
        {                    "SDL_CONTROLLERTOUCHPADDOWN",                     SDL_CONTROLLERTOUCHPADDOWN},
        {                  "SDL_CONTROLLERTOUCHPADMOTION",                   SDL_CONTROLLERTOUCHPADMOTION},
        {                      "SDL_CONTROLLERTOUCHPADUP",                       SDL_CONTROLLERTOUCHPADUP},
        {                    "SDL_CONTROLLERSENSORUPDATE",                     SDL_CONTROLLERSENSORUPDATE},
        {"SDL_CONTROLLERUPDATECOMPLETE_RESERVED_FOR_SDL3", SDL_CONTROLLERUPDATECOMPLETE_RESERVED_FOR_SDL3},
        {              "SDL_CONTROLLERSTEAMHANDLEUPDATED",               SDL_CONTROLLERSTEAMHANDLEUPDATED},
        {                                "SDL_FINGERDOWN",                                 SDL_FINGERDOWN},
        {                                  "SDL_FINGERUP",                                   SDL_FINGERUP},
        {                              "SDL_FINGERMOTION",                               SDL_FINGERMOTION},
        {                             "SDL_DOLLARGESTURE",                              SDL_DOLLARGESTURE},
        {                              "SDL_DOLLARRECORD",                               SDL_DOLLARRECORD},
        {                              "SDL_MULTIGESTURE",                               SDL_MULTIGESTURE},
        {                           "SDL_CLIPBOARDUPDATE",                            SDL_CLIPBOARDUPDATE},
        {                                  "SDL_DROPFILE",                                   SDL_DROPFILE},
        {                                  "SDL_DROPTEXT",                                   SDL_DROPTEXT},
        {                                 "SDL_DROPBEGIN",                                  SDL_DROPBEGIN},
        {                              "SDL_DROPCOMPLETE",                               SDL_DROPCOMPLETE},
        {                          "SDL_AUDIODEVICEADDED",                           SDL_AUDIODEVICEADDED},
        {                        "SDL_AUDIODEVICEREMOVED",                         SDL_AUDIODEVICEREMOVED},
        {                              "SDL_SENSORUPDATE",                               SDL_SENSORUPDATE},
        {                      "SDL_RENDER_TARGETS_RESET",                       SDL_RENDER_TARGETS_RESET},
        {                       "SDL_RENDER_DEVICE_RESET",                        SDL_RENDER_DEVICE_RESET},
        {                              "SDL_POLLSENTINEL",                               SDL_POLLSENTINEL},
        {                                 "SDL_USEREVENT",                                  SDL_USEREVENT},
        {                                 "SDL_LASTEVENT",                                  SDL_LASTEVENT},
        {                                            NULL,                                              0},
    };

    scripting_set_global_enum(g_L, sdl_events, "sdl_events");
}

void scripting_close()
{
    lua_close(g_L);
}

i32 get_lookup_id(u32 type)
{
    const u32 lookup_table[] = {
        SDL_FIRSTEVENT,
        SDL_QUIT,
        SDL_APP_TERMINATING,
        SDL_APP_LOWMEMORY,
        SDL_APP_WILLENTERBACKGROUND,
        SDL_APP_DIDENTERBACKGROUND,
        SDL_APP_WILLENTERFOREGROUND,
        SDL_APP_DIDENTERFOREGROUND,
        SDL_LOCALECHANGED,
        SDL_DISPLAYEVENT,
        SDL_WINDOWEVENT,
        SDL_SYSWMEVENT,
        SDL_KEYDOWN,
        SDL_KEYUP,
        SDL_TEXTEDITING,
        SDL_TEXTINPUT,
        SDL_KEYMAPCHANGED,
        SDL_TEXTEDITING_EXT,
        SDL_MOUSEMOTION,
        SDL_MOUSEBUTTONDOWN,
        SDL_MOUSEBUTTONUP,
        SDL_MOUSEWHEEL,
        SDL_JOYAXISMOTION,
        SDL_JOYBALLMOTION,
        SDL_JOYHATMOTION,
        SDL_JOYBUTTONDOWN,
        SDL_JOYBUTTONUP,
        SDL_JOYDEVICEADDED,
        SDL_JOYDEVICEREMOVED,
        SDL_JOYBATTERYUPDATED,
        SDL_CONTROLLERAXISMOTION,
        SDL_CONTROLLERBUTTONDOWN,
        SDL_CONTROLLERBUTTONUP,
        SDL_CONTROLLERDEVICEADDED,
        SDL_CONTROLLERDEVICEREMOVED,
        SDL_CONTROLLERDEVICEREMAPPED,
        SDL_CONTROLLERTOUCHPADDOWN,
        SDL_CONTROLLERTOUCHPADMOTION,
        SDL_CONTROLLERTOUCHPADUP,
        SDL_CONTROLLERSENSORUPDATE,
        SDL_CONTROLLERUPDATECOMPLETE_RESERVED_FOR_SDL3,
        SDL_CONTROLLERSTEAMHANDLEUPDATED,
        SDL_FINGERDOWN,
        SDL_FINGERUP,
        SDL_FINGERMOTION,
        SDL_DOLLARGESTURE,
        SDL_DOLLARRECORD,
        SDL_MULTIGESTURE,
        SDL_CLIPBOARDUPDATE,
        SDL_DROPFILE,
        SDL_DROPTEXT,
        SDL_DROPBEGIN,
        SDL_DROPCOMPLETE,
        SDL_AUDIODEVICEADDED,
        SDL_AUDIODEVICEREMOVED,
        SDL_SENSORUPDATE,
        SDL_RENDER_TARGETS_RESET,
        SDL_RENDER_DEVICE_RESET,
        SDL_POLLSENTINEL,
        SDL_USEREVENT,
        SDL_LASTEVENT,
        ENGINE_BLOCK_UDPATE,
        ENGINE_BLOCK_ERASED,
        ENGINE_BLOCK_CREATE,
        ENGINE_BLOB_UPDATE,
        ENGINE_BLOB_ERASED,
        ENGINE_BLOB_CREATE,
        ENGINE_TICK,
        ENGINE_INIT,
    };

    for (u32 i = 0; i < sizeof(lookup_table) / sizeof(u32); i++)
        if (type == lookup_table[i])
            return i;

    return -1;
}

int push_event_args(SDL_Event *e)
{
    block_update_event *block_event = NULL;
    blob_update_event *blob_event = NULL;
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
        lua_pushinteger(g_L, e->button.state);
        lua_pushinteger(g_L, e->button.clicks);
        lua_pushinteger(g_L, e->button.button);
        return 5;
    case SDL_MOUSEWHEEL:
        lua_pushinteger(g_L, e->wheel.x);
        lua_pushinteger(g_L, e->wheel.y);
        lua_pushinteger(g_L, e->wheel.mouseX);
        lua_pushinteger(g_L, e->wheel.mouseY);
        return 4;
    // case ENGINE_SPECIAL_SIGNAL:
    //     return 4;
    case ENGINE_BLOCK_UDPATE:
    case ENGINE_BLOCK_ERASED:
    case ENGINE_BLOCK_CREATE:
        block_event = (block_update_event *)e;

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
        blob_event = (blob_update_event *)e;

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
    i32 lookup_id = get_lookup_id(e.type);

    if (lookup_id == -1)
    {
        LOG_ERROR("Unknown event type %d", e.type);
        return;
    }

    vec_int_t target = handlers[lookup_id]; // Get the list of handlers for this event type

    const int handler_count = target.length;
    for (u32 i = 0; i < handler_count; i++)
    {
        lua_geti(g_L, LUA_REGISTRYINDEX, target.data[i]); // Get the function reference from the registry
        if (!lua_isfunction(g_L, -1))
        {
            LOG_ERROR("Handler %d for event type %d is not a function", target.data[i], lookup_id);
            lua_pop(g_L, 1); // Remove the non-function from the stack
            // maybe even unregister the handler so it doesn't get called again
            vec_remove(&target, i);
            continue;
        }
        // luaL_checktype(g_L, -1, LUA_TFUNCTION);

        u16 args = push_event_args(&e);

        // LOG_DEBUG("Calling handler %d for event type %d with %d args", target.data[i], lookup_id, args);

        if (lua_pcall(g_L, args, 0, 0) != 0)
        {
            LOG_ERROR("Error calling a handler, id %d: %s", lookup_id, lua_tostring(g_L, -1));
            lua_pop(g_L, 1);
        }
    }
}

void scripting_register_event_handler(int ref, int event_type)
{
    i32 lookup_id = get_lookup_id(event_type);

    LOG_DEBUG("registering %d handler, event id %d", lookup_id, event_type);

    if (lookup_id == sizeof(handlers))
    {
        LOG_ERROR("Unknown event type %d", event_type);
        return;
    }

    (void)vec_push(&handlers[lookup_id], ref);
}

/*
    Registers said input function to the block registry
*/
u8 scripting_register_block_input(block_registry *reg, u64 id, int ref, const char *name)
{
    CHECK_PTR(reg);
    CHECK_PTR(name);

    if (id > reg->resources.length)
    {
        LOG_ERROR("Invalid block id %d : out of range", id);
        return FAIL;
    }

    block_resources *res = &reg->resources.data[id];

    if (strcmp("tick", name) == 0)
    {
        LOG_DEBUG("registered the tick input for block %lld", id);
        res->input_tick_ref = ref;
    }

    for (u32 i = 0; i < res->input_names.length; i++)
        if (strcmp(res->input_names.data[i], name) == 0)
        {
            // res->input_refs.data[i] = ref;
            (void)vec_push(&res->input_refs, ref);
            return SUCCESS;
        }

    LOG_ERROR("Failed to register input %s", name);
    return FAIL;
}

int scripting_load_file(const char *reg_name, const char *short_filename)
{
    char filename[MAX_PATH_LENGTH] = {};
    snprintf(filename, MAX_PATH_LENGTH, FOLDER_REG SEPARATOR_STR "%s" SEPARATOR_STR FOLDER_REG_SCR SEPARATOR_STR "%s",
             reg_name, short_filename);
    int status = luaL_dofile(g_L, filename);

    if (status != LUA_OK)
    {
        fprintf(stderr, "Lua error in %s : %s", filename, lua_tostring(g_L, -1));
        return FAIL;
    }

    return SUCCESS;
}

u8 scripting_load_scripts(block_registry *registry)
{
    block_resources_t *reg = &registry->resources;
    const int length = reg->length;

    const char *reg_name = registry->name;

    for (u32 i = 0; i < length; i++)
    {
        block_resources *res = &reg->data[i];
        const char *lua_file = res->lua_script_filename;

        if (lua_file)
        {
            LOG_DEBUG("Loading script %s", lua_file);

            lua_pushinteger(g_L, res->id);
            lua_setglobal(g_L, "scripting_current_block_id");
            lua_pushcfunction(g_L, lua_light_block_input_register);
            lua_setglobal(g_L, "scripting_light_block_input_register");
            lua_pushlightuserdata(g_L, registry);
            lua_setglobal(g_L, "scripting_current_light_registry");

            scripting_load_file(reg_name, lua_file);
        }

        // checking if all inputs hav a handler

        if (res->input_names.length != res->input_refs.length)
        {
            LOG_ERROR("Block %d has %d inputs but %d handlers", res->id, res->input_names.length,
                      res->input_refs.length);
            return FAIL;
        }

        u8 failed = 0;

        for (u32 j = 0; j < res->input_refs.length; j++)
        {
            int func_ref = res->input_refs.data[j];
            if (func_ref == 0)
            {
                LOG_ERROR("Input %s of block %d has no handler, register it in "
                          "your lua script",
                          res->input_names.data[j], res->id);
                failed = 1;
            }
        }

        if (failed)
            return FAIL;
    }

    // cleaning up
    lua_pushnil(g_L);
    lua_setglobal(g_L, "scripting_current_block_id");
    lua_pushnil(g_L);
    lua_setglobal(g_L, "scripting_register_block_input");
    lua_pushnil(g_L);
    lua_setglobal(g_L, "scripting_current_light_registry");

    return SUCCESS;
}

// creates a lua table out of your array of poitners to enum_entries
void scripting_set_global_enum(lua_State *L, enum_entry entries[], const char *name)
{
    lua_newtable(L);

    u32 i = 0;
    while (entries[i].enum_entry_name != NULL)
    {
        enum_entry e = entries[i];
        lua_pushinteger(L, e.entry);
        lua_setfield(L, -2, e.enum_entry_name);

        i++;
    }

    lua_setglobal(L, name);
    LOG_DEBUG("Pushed enum %s", name);
}