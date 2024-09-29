#include "include/lua_integration.h"
#include "include/block_registry.h"

#include "include/lua_world_manipulation_functions.h"
#include "include/lua_block_data_functions.h"
#include "include/lua_utils.h"

lua_State *g_L = 0;

static int handlers[] = {
	1,					 // tick event
	2,					 // init event
	SDL_KEYDOWN,		 // 768
	SDL_KEYUP,			 // 769
	SDL_MOUSEBUTTONDOWN, // 1025
	SDL_MOUSEBUTTONUP	 // 1026

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

	*/
	const static luaL_Reg blockengine_lib[] = {
		{"access_block", lua_access_block},
		{"set_block", lua_set_block},
		{"clean_block", lua_clean_block},
		{"move_block_gently", lua_move_block_gently},
		{"move_block_rough", lua_move_block_rough},
		{"move_block_recursive", lua_move_block_recursive},
		{"is_data_equal", lua_is_data_equal},
		{"is_block_void", lua_is_block_void},
		{"is_block_equal", lua_is_block_equal},
		{"is_chunk_equal", lua_is_chunk_equal},
		{"block_data_free", lua_block_data_free},
		{"block_erase", lua_block_erase},
		{"block_copy", lua_block_copy},
		{"block_init", lua_block_init},
		{"block_teleport", lua_block_teleport},
		{"block_swap", lua_block_swap},

		{"blob_create", lua_blob_create},
		{"blob_remove", lua_blob_remove},
		{"blob_set_string", lua_blob_set_str},
		{"blob_set_int", lua_blob_set_i},
		{"blob_set_sshort", lua_blob_set_s},
		{"blob_set_byte", lua_blob_set_b},
		{"blob_get_string", lua_blob_get_str},
		{"blob_get_int", lua_blob_get_i},
		{"blob_get_short", lua_blob_get_s},
		{"blob_get_byte", lua_blob_get_b},

		{"block_unpack", block_unpack},
		{"get_keyboard_state", get_keyboard_state},

		{NULL, NULL}

	};

	luaL_newlib(L, blockengine_lib); // creates a table with blockengine functions
	lua_setglobal(L, "blockengine"); // sets the table as global variable "blockengine"
									 // 0 objects on stack now
}

void scripting_define_global_variables(const world *w)
{
	lua_pushlightuserdata(g_L, (void *)w);
	lua_setglobal(g_L, "g_world");
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
	lua_setglobal(g_L, "event_handlers"); /* makes the event_handlers table availiable for everyone */
}

void scripting_close()
{
	lua_close(g_L);
}

int push_event_args(SDL_Event *e, const int event_id)
{
	if (!e)
		return 0;

	switch (event_id)
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
	}
	return 0;
}

int scripting_handle_event(SDL_Event *event, const int override_id)
{
	const int event_id = event ? event->type : override_id;

	lua_getglobal(g_L, "event_handlers");

	lua_pushnil(g_L);
	while (lua_next(g_L, -2) != 0)
	{
		int id = lua_tonumber(g_L, -2);
		if (lua_type(g_L, -1) == LUA_TTABLE && id == event_id)
		{
			lua_pushnil(g_L);
			while (lua_next(g_L, -2) != 0)
			{
				if (lua_type(g_L, -1) == LUA_TFUNCTION)
					if (lua_pcall(g_L, push_event_args(event, event_id), 0, 0) != 0)
						printf("error running function `f': %s\n", lua_tostring(g_L, -1));
					else
						;
				else
					lua_pop(g_L, 1);
			}
		}

		lua_pop(g_L, 1);
	}

	lua_pop(g_L, 1);

	return 0;
}

int scripting_load_file(const char *short_filename)
{
	/*
	the function is expected to return a table, that contains a list of
	all events, that this script is listening to, and a function that
	will be called for each event

	format of the table:

	{
		[EVENT_ID] = function(event_data)
			-- do something
		end
	}

	list of all possible events can be found in our docs
	*/

	char filename[MAX_PATH_LENGTH] = "resources/scripts/";
	strcat(filename, short_filename);

	int status = luaL_dofile(g_L, filename);

	if (status != LUA_OK)
	{
		fprintf(stderr, "Lua error in %s : %s", filename, lua_tostring(g_L, -1));
		lua_pop(g_L, 1); /* pop error message from the stack */
		return FAIL;
	}

	// // lets get this table from the top of the stack
	// int elements = lua_gettop(g_L);
	// int type = lua_type(g_L, -1);

	// if (type != LUA_TTABLE)
	// {
	// 	fprintf(stderr, "Lua error in %s : expected table at the top of the stack, got %s instead\n", filename, lua_typename(g_L, type));
	// 	lua_pop(g_L, 1); /* pop error message from the stack */
	// 	return FAIL;
	// }

	// lua_getglobal(g_L, "event_handlers");

	// // iterate over the table and get all functions
	// lua_pushnil(g_L);
	// while (lua_next(g_L, -3) != 0) // -3 is returned table from a script, -2 is event handlers table
	// {
	// 	// check for correct types
	// 	if (lua_type(g_L, -1) != LUA_TFUNCTION || lua_type(g_L, -2) != LUA_TNUMBER)
	// 	{
	// 		fprintf(stderr, "Lua error in %s : expected function and event id (number), got %s and %s instead\n", filename, lua_typename(g_L, lua_type(g_L, -1)), lua_typename(g_L, lua_type(g_L, -2)));
	// 		continue;
	// 	}

	// 	// damn i cant do this
	// 	// scripting_register_event(lua_tocfunction(g_L, -1), lua_tonumber(g_L, -2));

	// 	lua_pop(g_L, 1);
	// }

	// // pop all elements from the stack
	// lua_pop(g_L, elements);

	return SUCCESS;
}

void scripting_load_scripts(block_registry_t *reg)
{
	const int length = reg->length;

	for (int i = 0; i < length; i++)
	{
		const char *lua_file = reg->data[i].lua_script_filename;

		if (lua_file)
			scripting_load_file(lua_file);
	}
}
