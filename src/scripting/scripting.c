#include "include/scripting.h"
// #include "include/image_editing.h"
#include "SDL_events.h"
#include "include/events.h"
#include "include/file_system.h"
#include "include/folder_structure.h"
#include "include/logging.h"
#include "include/scripting_bindings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

lua_State *g_L = 0;

vec_int_t handlers[128] = {};

/* module container magic */
#define MOD_CONTAINER_MAGIC "MODL"

typedef struct module_blob_t
{
    char *name;
    unsigned char *data;
    u32 size;
} module_blob_t;

static int module_list_find(module_blob_t *list, u32 count, const char *name)
{
    for (u32 i = 0; i < count; i++)
        if (strcmp(list[i].name, name) == 0)
            return (int)i;
    return -1;
}

/* append u32 little-endian to buffer (grow via realloc) */
static int append_u32(unsigned char **buf, size_t *cap, size_t *len, u32 v)
{
    if (*len + 4 > *cap)
    {
        *cap = (*len + 4) * 2;
        unsigned char *nb = realloc(*buf, *cap);
        if (!nb)
            return -1;
        *buf = nb;
    }
    (*buf)[(*len) + 0] = (unsigned char)(v & 0xFF);
    (*buf)[(*len) + 1] = (unsigned char)((v >> 8) & 0xFF);
    (*buf)[(*len) + 2] = (unsigned char)((v >> 16) & 0xFF);
    (*buf)[(*len) + 3] = (unsigned char)((v >> 24) & 0xFF);
    *len += 4;
    return 0;
}

static int append_bytes(unsigned char **buf, size_t *cap, size_t *len, const void *data, size_t data_len)
{
    if (*len + data_len > *cap)
    {
        *cap = (*len + data_len) * 2;
        unsigned char *nb = realloc(*buf, *cap);
        if (!nb)
            return -1;
        *buf = nb;
    }
    memcpy(*buf + *len, data, data_len);
    *len += data_len;
    return 0;
}

/* writer state for lua_dump (defined early so collectors can use it) */
struct dump_state
{
    unsigned char *buf;
    size_t size;
};

static int lua_dump_writer(lua_State *L, const void *p, size_t sz, void *ud)
{
    struct dump_state *st = (struct dump_state *)ud;
    unsigned char *newbuf = realloc(st->buf, st->size + sz);
    if (!newbuf)
        return 1; // memory error
    st->buf = newbuf;
    memcpy(st->buf + st->size, p, sz);
    st->size += sz;
    return 0;
}

static char *read_entire_file(const char *filename, size_t *out_size)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0)
    {
        fclose(f);
        return NULL;
    }
    char *buf = malloc((size_t)sz + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    size_t read = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[read] = '\0';
    if (out_size)
        *out_size = read;
    return buf;
}

static int collect_module_recursive(const char *modname, module_blob_t **out_list, u32 *out_count);

static void scan_and_collect_requires(const char *source, module_blob_t **out_list, u32 *out_count)
{
    const char *p = source;
    while (*p)
    {
        const char *req = strstr(p, "require");
        if (!req)
            break;
        const char *q = req + strlen("require");
        while (*q && isspace((unsigned char)*q))
            q++;
        if (*q != '(')
        {
            p = q;
            continue;
        }
        q++;
        while (*q && isspace((unsigned char)*q))
            q++;
        if (*q != '"' && *q != '\'')
        {
            p = q;
            continue;
        }
        char quote = *q++;
        const char *start = q;
        const char *end = strchr(start, quote);
        if (!end)
        {
            p = q;
            continue;
        }
        size_t modlen = end - start;
        char *modname = malloc(modlen + 1);
        strncpy(modname, start, modlen);
        modname[modlen] = '\0';

        collect_module_recursive(modname, out_list, out_count);

        free(modname);

        p = end + 1;
    }
}

static int collect_module_recursive(const char *modname, module_blob_t **out_list, u32 *out_count)
{
    if (module_list_find(*out_list, *out_count, modname) != -1)
        return 0; /* already collected */

    /* build path */
    char modpath[MAX_PATH_LENGTH] = {};
    size_t modlen = strlen(modname);
    size_t i;
    for (i = 0; i < modlen && i + 6 < MAX_PATH_LENGTH; i++)
        modpath[i] = (modname[i] == '.') ? '/' : modname[i];
    modpath[i] = '\0';
    strncat(modpath, ".lua", MAX_PATH_LENGTH - strlen(modpath) - 1);

    size_t src_size = 0;
    char *src = read_entire_file(modpath, &src_size);
    if (!src)
    {
        LOG_WARNING("Could not read module %s at path %s", modname, modpath);
        return -1;
    }

    /* scan nested requires first */
    scan_and_collect_requires(src, out_list, out_count);

    /* compile module source to bytecode */
    int status = luaL_loadbuffer(g_L, src, strlen(src), modname);
    if (status != LUA_OK)
    {
        LOG_WARNING("Could not compile module %s: %s", modname, lua_tostring(g_L, -1));
        lua_pop(g_L, 1);
        free(src);
        return -1;
    }

    struct dump_state st = {malloc(BUFFER_SIZE), 0};
    if (lua_dump(g_L, lua_dump_writer, &st, 0) != 0)
    {
        free(st.buf);
        lua_pop(g_L, 1);
        free(src);
        return -1;
    }
    lua_pop(g_L, 1);

    /* append to list */
    module_blob_t *nl = realloc(*out_list, sizeof(module_blob_t) * (*out_count + 1));
    if (!nl)
    {
        free(st.buf);
        free(src);
        return -1;
    }
    *out_list = nl;
    (*out_list)[*out_count].name = strdup(modname);
    (*out_list)[*out_count].data = st.buf;
    (*out_list)[*out_count].size = (u32)st.size;
    (*out_count)++;

    free(src);
    return 0;
}

/* forward declarations used by preprocess helpers */
struct dump_state;
static int lua_dump_writer(lua_State *L, const void *p, size_t sz, void *ud);

static int lua_register_handler(lua_State *L)
{
    int event_id = luaL_checkinteger(L, 1);
    luaL_checkany(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    scripting_register_event_handler(ref, event_id);
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
    if (lua_islightuserdata(L, index))
        return lua_touserdata(L, index);
    luaL_error(L, "Expected light userdata at index %d", index);
    return NULL;
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
        {  "ENGINE_INIT_GLOBALS",   ENGINE_INIT_GLOBALS},

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
        ENGINE_INIT_GLOBALS,
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
    case SDL_WINDOWEVENT:
        if (e->window.event == SDL_WINDOWEVENT_RESIZED || e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            lua_pushinteger(g_L, e->window.data1);
            lua_pushinteger(g_L, e->window.data2);
            return 2;
        }
        else
            return 0;
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
    assert(reg);
    assert(name);

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

    LOG_ERROR("Input \"%s\" not found in block resources. All available inputs:", name);
    for (u32 i = 0; i < res->input_names.length; i++)
        LOG_ERROR("%s", res->input_names.data[i]);

    return FAIL;
}

int scripting_do_script(const char *reg_name, const char *short_filename)
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
    const char *reg_name = registry->name;

    for (u32 i = 0; i < reg->length; i++)
    {
        block_resources *res = &reg->data[i];
        const char *lua_file = res->lua_script_filename;

        lua_pushinteger(g_L, res->id);
        lua_setglobal(g_L, "scripting_current_block_id");
        lua_pushcfunction(g_L, lua_light_block_input_register);
        lua_setglobal(g_L, "scripting_light_block_input_register");
        lua_pushlightuserdata(g_L, registry);
        lua_setglobal(g_L, "scripting_current_light_registry");

        if (res->lua_script_blob != NULL && res->lua_script_blob_size > 0) // embedded bytecode load
        {
            LOG_DEBUG("Loading embedded script %s (bytecode)",
                      res->lua_script_filename ? res->lua_script_filename : "<anon>");

            if (scripting_load_compiled_blob(reg_name,
                                             res->lua_script_filename ? res->lua_script_filename : "<embedded>",
                                             res->lua_script_blob, res->lua_script_blob_size) != SUCCESS)
            {
                LOG_ERROR("Failed to load embedded script for block %d", res->id);
                return FAIL;
            }

            /* free blob after successful load to avoid holding extra memory */
            free(res->lua_script_blob);
            res->lua_script_blob = NULL;
            res->lua_script_blob_size = 0;
        }
        else if (lua_file) // legacy source file load
        {
            LOG_DEBUG("Loading script %s", lua_file);

            scripting_do_script(reg_name, lua_file);

            /* free blob after successful load to avoid holding extra memory */
            free(res->lua_script_blob);
            res->lua_script_blob = NULL;
            res->lua_script_blob_size = 0;
        }

        // checking if all inputs hav a handler
        // should match if the script actually registered them
        if (res->input_names.length != res->input_refs.length)
        {
            LOG_ERROR("Block %d has %d inputs but %d handlers", res->id, res->input_names.length,
                      res->input_refs.length);
            return FAIL;
        }

        bool failed = false;

        for (u32 j = 0; j < res->input_refs.length; j++)
        {
            int func_ref = res->input_refs.data[j];
            if (func_ref == 0)
            {
                LOG_ERROR("Input %s of block %d has no handler, register it in "
                          "your lua script",
                          res->input_names.data[j], res->id);
                failed = true;
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

    

int scripting_compile_file_to_bytecode(const char *reg_name, const char *short_filename, unsigned char **out_buf,
                                       u32 *out_size)
{
    char filename[MAX_PATH_LENGTH] = {};
    snprintf(filename, MAX_PATH_LENGTH, FOLDER_REG SEPARATOR_STR "%s" SEPARATOR_STR FOLDER_REG_SCR SEPARATOR_STR "%s",
             reg_name, short_filename);

    /* read source and collect required modules (compile them into blobs) */
    size_t src_size = 0;
    char *src = read_entire_file(filename, &src_size);
    if (!src)
    {
        fprintf(stderr, "Failed to read lua source %s\n", filename);
        return FAIL;
    }

    module_blob_t *modules = NULL;
    u32 module_count = 0;
    scan_and_collect_requires(src, &modules, &module_count);

    /* compile main script */
    int status = luaL_loadbuffer(g_L, src, strlen(src), short_filename);
    free(src);
    if (status != LUA_OK)
    {
        fprintf(stderr, "Lua compile error in %s : %s\n", filename, lua_tostring(g_L, -1));
        lua_pop(g_L, 1);
        /* free collected modules */
        for (u32 i = 0; i < module_count; i++)
        {
            free(modules[i].name);
            free(modules[i].data);
        }
        free(modules);
        return FAIL;
    }

    struct dump_state st_main = {malloc(BUFFER_SIZE), 0};
    if (lua_dump(g_L, lua_dump_writer, &st_main, 0) != 0)
    {
        free(st_main.buf);
        for (u32 i = 0; i < module_count; i++)
        {
            free(modules[i].name);
            free(modules[i].data);
        }
        free(modules);
        return FAIL;
    }
    lua_pop(g_L, 1);

    /* build container buffer: magic, module_count, [name_len,name,data_len,data]..., main_len, main_data */
    unsigned char *buf = malloc(1024);
    size_t cap = 1024;
    size_t len = 0;
    /* magic */
    append_bytes(&buf, &cap, &len, MOD_CONTAINER_MAGIC, 4);
    append_u32(&buf, &cap, &len, module_count);
    for (u32 i = 0; i < module_count; i++)
    {
        u32 name_len = (u32)strlen(modules[i].name);
        append_u32(&buf, &cap, &len, name_len);
        append_bytes(&buf, &cap, &len, modules[i].name, name_len);
        append_u32(&buf, &cap, &len, modules[i].size);
        append_bytes(&buf, &cap, &len, modules[i].data, modules[i].size);
    }
    append_u32(&buf, &cap, &len, (u32)st_main.size);
    append_bytes(&buf, &cap, &len, st_main.buf, st_main.size);

    /* cleanup module list and main dump buffer ownership transferred to container */
    free(st_main.buf);
    for (u32 i = 0; i < module_count; i++)
    {
        free(modules[i].name);
        free(modules[i].data);
    }
    free(modules);

    *out_buf = buf;
    *out_size = (u32)len;
    return SUCCESS;
}

int scripting_load_compiled_blob(const char *reg_name, const char *short_filename, const unsigned char *data, u32 size)
{
    assert(reg_name);
    assert(short_filename);
    assert(data);
    assert(size > 0);

    LOG_DEBUG("Loading compiled lua blob for registry %s, script %s, size %u bytes", reg_name, short_filename, size);

    /* detect our MODL container format */
    if (size >= 4 && memcmp(data, MOD_CONTAINER_MAGIC, 4) == 0)
    {
        size_t pos = 4;
        if (pos + 4 > size)
        {
            LOG_ERROR("Malformed module container for %s", short_filename);
            return FAIL;
        }
        u32 module_count = (u32)(data[pos] | (data[pos+1]<<8) | (data[pos+2]<<16) | (data[pos+3]<<24));
        pos += 4;

        for (u32 i = 0; i < module_count; i++)
        {
            if (pos + 4 > size) { LOG_ERROR("Malformed module container"); return FAIL; }
            u32 name_len = (u32)(data[pos] | (data[pos+1]<<8) | (data[pos+2]<<16) | (data[pos+3]<<24));
            pos += 4;
            if (pos + name_len > size) { LOG_ERROR("Malformed module container"); return FAIL; }
            char *name = malloc(name_len + 1);
            memcpy(name, data + pos, name_len);
            name[name_len] = '\0';
            pos += name_len;

            if (pos + 4 > size) { free(name); LOG_ERROR("Malformed module container"); return FAIL; }
            u32 mod_size = (u32)(data[pos] | (data[pos+1]<<8) | (data[pos+2]<<16) | (data[pos+3]<<24));
            pos += 4;
            if (pos + mod_size > size) { free(name); LOG_ERROR("Malformed module container"); return FAIL; }

            const unsigned char *mod_data = data + pos;
            pos += mod_size;

            /* load module chunk and register into package.preload[name] */
            int status = luaL_loadbuffer(g_L, (const char *)mod_data, (size_t)mod_size, name);
            if (status != LUA_OK)
            {
                LOG_ERROR("Lua loadbuffer error for preload %s: %s", name, lua_tostring(g_L, -1));
                lua_pop(g_L, 1);
                free(name);
                return FAIL;
            }

            /* stack: function */
            lua_getglobal(g_L, "package"); /* function, package */
            lua_getfield(g_L, -1, "preload"); /* function, package, preload */
            lua_pushvalue(g_L, -3); /* copy function */
            lua_setfield(g_L, -2, name); /* preload[name] = function */
            lua_pop(g_L, 3); /* pop preload, package, original function */

            free(name);
        }

        /* now remaining bytes are main script */
        if (pos + 4 > size) { LOG_ERROR("Malformed module container end"); return FAIL; }
        u32 main_size = (u32)(data[pos] | (data[pos+1]<<8) | (data[pos+2]<<16) | (data[pos+3]<<24));
        pos += 4;
        if (pos + main_size > size) { LOG_ERROR("Malformed module container main"); return FAIL; }

        int status = luaL_loadbuffer(g_L, (const char *)(data + pos), (size_t)main_size, short_filename ? short_filename : "<embedded>");
        if (status != LUA_OK)
        {
            LOG_ERROR("Lua loadbuffer error for %s: %s", short_filename ? short_filename : "<embedded>", lua_tostring(g_L, -1));
            lua_pop(g_L, 1);
            return FAIL;
        }
        if (lua_pcall(g_L, 0, 0, 0) != LUA_OK)
        {
            LOG_ERROR("Lua pcall error for %s: %s", short_filename ? short_filename : "<embedded>", lua_tostring(g_L, -1));
            lua_pop(g_L, 1);
            return FAIL;
        }

        return SUCCESS;
    }

    /* legacy: single blob */
    int status = luaL_loadbuffer(g_L, (const char *)data, (size_t)size, short_filename ? short_filename : "<embedded>");
    if (status != LUA_OK)
    {
        LOG_ERROR("Lua loadbuffer error for %s: %s", short_filename ? short_filename : "<embedded>",
                  lua_tostring(g_L, -1));
        lua_pop(g_L, 1);
        return FAIL;
    }

    if (lua_pcall(g_L, 0, 0, 0) != LUA_OK)
    {
        LOG_ERROR("Lua pcall error for %s: %s", short_filename ? short_filename : "<embedded>", lua_tostring(g_L, -1));
        lua_pop(g_L, 1);
        return FAIL;
    }

    return SUCCESS;
}