#include "../include/scripting.h"

lua_State *g_L = 0;

static int handlers[] = {
    1,                   // tick event
    2,                   // init event
    SDL_KEYDOWN,         // 768
    SDL_KEYUP,           // 769
    SDL_MOUSEBUTTONDOWN, // 1025
    SDL_MOUSEBUTTONUP    // 1026

    //{SDL_MOUSEMOTION,{}},
    //{SDL_MOUSEWHEEL,{}},
    //{SDL_WINDOWEVENT, {}},
    //{SDL_TEXTINPUT, {}},
    //{SDL_TEXTEDITING, {}}

};

#define SUPPORTED_EVENTS sizeof(handlers) / sizeof(handlers[0])

void scripting_register(lua_State *L)
{
    /*
    TODO:

    add block data editig functions

    int lua_set_block_id(lua_State *L);
    int lua_get_block_id(lua_State *L);

    int lua_get_block_vars(lua_State *L);
    int lua_vars_get_integer(lua_State *L);

    int lua_vars_set_integer(lua_State *L);

    */
    const static luaL_Reg blockengine_lib[] = {

        {"set_block_id", lua_set_block_id},
        {"get_block_id", lua_get_block_id},

        {"get_block_vars", lua_get_block_vars},

        {"vars_get_integer", lua_vars_get_integer},
        {"vars_set_integer", lua_vars_set_integer},

        {NULL, NULL}

    };

    luaL_newlib(L, blockengine_lib); // creates a table with blockengine functions
    lua_setglobal(L, "blockengine"); // sets the table as global variable "blockengine"
                                     // 0 objects on stack now
}

void scripting_check_arguments(lua_State *L, int num, ...)
{
    va_list valist;
    va_start(valist, num);

    for (int i = 1; i <= num; i++)
        LUA_ARG_CHECK(L, i, va_arg(valist, int))

    va_end(valist);
}

void scripting_define_global_object(void *ptr, char *name)
{
    lua_pushlightuserdata(g_L, ptr);
    lua_setglobal(g_L, name);
}

void scripting_init()
{
    g_L = luaL_newstate(); /* opens Lua */
    luaL_openlibs(g_L);
    scripting_register(g_L); /* register all blockengine functions */
    //
    lua_createtable(g_L, 0, SUPPORTED_EVENTS);
    for (int i = 0; i < SUPPORTED_EVENTS; i++)
    {
        lua_pushinteger(g_L, handlers[i]);
        lua_newtable(g_L);
        lua_settable(g_L, -3);
    }

    lua_setglobal(g_L, "event_handlers"); /* makes the event_handlers table availiable for everyone to register their stuff */
}

void scripting_close()
{
    lua_close(g_L);
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
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        lua_pushinteger(g_L, e->button.x);
        lua_pushinteger(g_L, e->button.y);
        lua_pushinteger(g_L, e->button.button);
        lua_pushinteger(g_L, e->button.state);
        lua_pushinteger(g_L, e->button.clicks);
        return 5;
    case ENGINE_BLOCK_UDPATE:
    case ENGINE_BLOCK_ERASED:
    case ENGINE_BLOCK_CREATE:
        block_update_event *block_event = (block_update_event *)e;

        lua_pushlightuserdata(g_L, block_event->room_ptr);
        lua_pushlightuserdata(g_L, block_event->layer_ptr);

        lua_pushinteger(g_L, block_event->target_id);
        lua_pushinteger(g_L, block_event->previous_id);

        lua_pushinteger(g_L, block_event->x);
        lua_pushinteger(g_L, block_event->y);

        return 6;
    case ENGINE_BLOB_UPDATE:
    case ENGINE_BLOB_ERASED:
    case ENGINE_BLOB_CREATE:
        blob_update_event *blob_event = (blob_update_event *)e;

        lua_pushlightuserdata(g_L, blob_event->room_ptr);
        lua_pushlightuserdata(g_L, blob_event->layer_ptr);

        lua_pushinteger(g_L, blob_event->target_id);

        lua_pushinteger(g_L, blob_event->x);
        lua_pushinteger(g_L, blob_event->y);

        lua_pushinteger(g_L, blob_event->pos);

        buf[0] = blob_event->letter;
        lua_pushstring(g_L, buf);
        lua_pushinteger(g_L, blob_event->size);

        lua_pushlightuserdata(g_L, blob_event->ptr);

        return 9;
    }
    return 0;
}

void call_handler(SDL_Event *e)
{
    if (lua_pcall(g_L, push_event_args(e), 0, 0) != 0)
        printf("error calling a handler `f': %s\n", lua_tostring(g_L, -1));
}

void scripting_register_event(lua_CFunction function, const int event_id)
{
    // TODO
}

int scripting_handle_event(SDL_Event *event, const int override_id)
{
    const int event_id = event ? event->type : override_id;

    lua_getglobal(g_L, "event_handlers"); // get table of event handlers, contains table of handers where keys is block ids

    lua_pushinteger(g_L, event_id); // get a table of handlers for certain event_id
    lua_gettable(g_L, -2);

    if (lua_istable(g_L, -1)) // if any handlers exists here
    {
        if (IS_ENGINE_EVENT(event_id)) // if event is for internal engine purposes only
        {
            if (IS_BLOCK_EVENT(event_id)) // for block events
            {
                block_update_event *real_event = (block_update_event *)event;
                // select a new id to call needed handler
                // lua_pushinteger(g_L, real_event->new_id);
                // select a prev id to call needed handler
                lua_pushinteger(g_L, real_event->previous_id);
                lua_gettable(g_L, -2);

                if (lua_isfunction(g_L, -1)) // call, if exists
                    call_handler(event);
                else
                    lua_pop(g_L, 1);
            }
        }
        else // if event is just a general sdl2 event, iterate through all handlers and call them all
        {
            lua_pushnil(g_L);
            while (lua_next(g_L, -2) != 0)
            {
                if (lua_isfunction(g_L, -1)) // call, if exists
                    call_handler(event);
                else
                    lua_pop(g_L, 1);
            }
        }
    }

    lua_pop(g_L, 2); // pop a table and his event_id out of my lua stack

    return 0;
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

        lua_pushinteger(g_L, reg->data[i].id);
        lua_setglobal(g_L, "scripting_current_block_id");

        if (lua_file)
            scripting_load_file(reg_name, lua_file);
    }
}
