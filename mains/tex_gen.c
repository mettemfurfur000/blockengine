#include "include/scripting.h"
// #include "include/block_registry.h"
// #include "include/level.h"
// #include "include/rendering.h"
// #include "include/vars.h"


#include "include/scripting_bindings.h"
#include <lua.h>

const char *usage = "Usage: %s <script_name> <args>\n";

int main(int argc, char *argv[])
{
    log_start("texturegen.log");

    if (argc < 2)
    {
        printf(usage, argv[0]);
        return 0;
    }

    char *script_name = argv[1];

    g_L = luaL_newstate(); /* opens Lua */
    luaL_openlibs(g_L);

    image_load_editing_library(g_L);

    lua_newtable(g_L);

    for(u32 i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--script") == 0)
        {
            i++; // skip the script name
            continue;
        }

        lua_pushstring(g_L, argv[i]);
        lua_rawseti(g_L, -2, i);
    }
    lua_setglobal(g_L, "c_args");

    scripting_do_script("texgen", script_name);

    scripting_close();

    return 0;
}