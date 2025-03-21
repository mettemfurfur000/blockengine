#include "../include/vars.h"
#include "../include/level.h"
#include "../include/rendering.h"
#include "../include/block_registry.h"
#include "../include/scripting.h"

#include "../include/image_editing.h"

const char *usage = "Usage: %s --script <script_name> --input <input_name> --output <output_name>\n";

int main(int argc, char *argv[])
{
	log_start("texturegen.log");

	if (argc < 5)
	{
		printf(usage, argv[0]);
		return 0;
	}

	char *script_name = NULL;
	char *output_name = NULL;
	char *input_name = NULL;

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "--script") == 0)
			script_name = argv[i + 1];
		else if (strcmp(argv[i], "--output") == 0)
			output_name = argv[i + 1];
		else if (strcmp(argv[i], "--input") == 0)
			input_name = argv[i + 1];
	}

	if (script_name == NULL || output_name == NULL || input_name == NULL)
	{
		printf(usage, argv[0]);
		return 0;
	}

	g_L = luaL_newstate(); /* opens Lua */
	luaL_openlibs(g_L);

	load_image_editing_library(g_L);

	lua_pushstring(g_L, output_name);
	lua_setglobal(g_L, "output_file");

	lua_pushstring(g_L, input_name);
	lua_setglobal(g_L, "input_file");

	scripting_load_file("texgen", script_name);

	scripting_close();

	return 0;
}