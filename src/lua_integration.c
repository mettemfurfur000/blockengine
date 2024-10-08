#include "include/lua_integration.h"
#include "include/block_registry.h"

#include "include/lua_drawing_functions.h"
#include "include/lua_world_manipulation_functions.h"
#include "include/lua_block_data_functions.h"
#include "include/lua_registry_functions.h"
#include "include/lua_utils.h"

#include "include/engine_events.h"

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
		{"blob_set_short", lua_blob_set_s},
		{"blob_set_byte", lua_blob_set_b},

		{"blob_get_string", lua_blob_get_str},
		{"blob_get_int", lua_blob_get_i},
		{"blob_get_short", lua_blob_get_s},
		{"blob_get_byte", lua_blob_get_b},

		{"blob_set_number", lua_blob_set_number},
		{"blob_get_number", lua_blob_get_number},

		{"block_unpack", block_unpack},
		{"get_keyboard_state", get_keyboard_state},

		{"read_registry_entry", lua_read_registry_entry},

		{"render_rules_get_resolutions", lua_render_rules_get_resolutions},
		{"render_rules_get_order", lua_render_rules_get_order},

		{"render_rules_slice_get", lua_slice_get},
		{"render_rules_slice_set", lua_slice_set},

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
	case ENGINE_BLOCK_SET:
	case ENGINE_BLOCK_ERASED:
	case ENGINE_BLOCK_UDPATE:
	case ENGINE_BLOCK_MOVE:
		block_update_event *real_event = (block_update_event *)e;

		lua_pushlightuserdata(g_L, real_event->target);

		lua_pushinteger(g_L, real_event->target_x);
		lua_pushinteger(g_L, real_event->target_y);

		lua_pushinteger(g_L, real_event->target_layer_id);
		lua_pushinteger(g_L, real_event->previous_id);
		lua_pushinteger(g_L, real_event->new_id);

		return 6;
		// case ENGINE_BLOB_UPDATE:
		// 	engine_event *engn_e = (engine_event *)e;

		// 	lua_pushlightuserdata(g_L, engn_e->target);
		// 	lua_pushlightuserdata(g_L, engn_e->actor);

		// 	lua_pushinteger(g_L, engn_e->target_x);
		// 	lua_pushinteger(g_L, engn_e->target_y);

		// 	lua_pushinteger(g_L, engn_e->actor_x);
		// 	lua_pushinteger(g_L, engn_e->actor_y);

		// 	lua_pushinteger(g_L, engn_e->target_layer_id);
		// 	lua_pushinteger(g_L, engn_e->actor_layer_id);

		// 	lua_pushinteger(g_L, engn_e->blob_letter);
		// 	lua_pushinteger(g_L, engn_e->reserve_flags);

		// 	lua_pushlstring(g_L, (char *)engn_e->signal, sizeof(engn_e->signal));

		// 	return 11;
	}
	return 0;
}

void call_handler(SDL_Event *e)
{
	if (lua_pcall(g_L, push_event_args(e), 0, 0) != 0)
		printf("error calling a handler `f': %s\n", lua_tostring(g_L, -1));
}

int scripting_handle_event(SDL_Event *event, const int override_id)
{
	const int event_id = event ? event->type : override_id;

	lua_getglobal(g_L, "event_handlers"); // get table of event handlers, contains table of handers where keys is block ids

	lua_pushinteger(g_L, event_id); // get a table of handlers for certain event_id
	lua_gettable(g_L, -2);

	if (lua_istable(g_L, -1)) // if any handlers exists here
	{
		if (is_user_event(event_id)) // if event is for internal engine purposes only
		{
			if (is_block_event(event_id)) // for block events
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

int scripting_load_file(const char *short_filename)
{
	char filename[MAX_PATH_LENGTH] = "resources/scripts/";
	strcat(filename, short_filename);
	int status = luaL_dofile(g_L, filename);

	if (status != LUA_OK)
	{
		fprintf(stderr, "Lua error in %s : %s", filename, lua_tostring(g_L, -1));
		return FAIL;
	}

	return SUCCESS;
}

void scripting_load_scripts(block_registry_t *reg)
{
	const int length = reg->length;

	for (int i = 0; i < length; i++)
	{
		const char *lua_file = reg->data[i].lua_script_filename;

		lua_pushinteger(g_L, reg->data[i].block_sample.id);
		lua_setglobal(g_L, "scripting_current_block_id");

		if (lua_file)
			scripting_load_file(lua_file);
	}
}
