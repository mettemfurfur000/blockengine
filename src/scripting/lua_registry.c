#include "include/scripting/registry.h"

#include "include/scripting.h"
#include "include/scripting_bindings.h"
#include <lua.h>

static int lua_registry_get_name(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, BlockRegistry, wrapper, 1);

	lua_pushstring(L, wrapper->reg->name);
	return 1;
}

static int lua_registry_to_table(lua_State *L)
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

		image *img = r.img;

		if (img)
		{
			lua_newtable(L);
			{
				// STRUCT_GET(L, (*img), filename, lua_pushstring);
				// STRUCT_GET(L, (*img), gl_id, lua_pushinteger);
				STRUCT_GET(L, (*img), width, lua_pushinteger);
				STRUCT_GET(L, (*img), height, lua_pushinteger);
				// STRUCT_GET(L, (*img), frames, lua_pushinteger);
				// STRUCT_GET(L, (*img), types, lua_pushinteger);
				// STRUCT_GET(L, (*img), total_frames, lua_pushinteger);
			}
			lua_setfield(L, -2, "texture");
		}

		lua_newtable(L);
		{
			STRUCT_GET(L, r.info, width, lua_pushinteger);
			STRUCT_GET(L, r.info, height, lua_pushinteger);
			STRUCT_GET(L, r.info, atlas_offset_x, lua_pushinteger);
			STRUCT_GET(L, r.info, atlas_offset_y, lua_pushinteger);
			STRUCT_GET(L, r.info, frames, lua_pushinteger);
			STRUCT_GET(L, r.info, types, lua_pushinteger);
			STRUCT_GET(L, r.info, total_frames, lua_pushinteger);
		}
		lua_setfield(L, -2, "atlas_info");

		if (r.sounds.length > 0)
		{
			lua_newtable(L);

			sound s;
			u32 j;
			vec_foreach(&r.sounds, s, j)
			{
				lua_newtable(L);
				STRUCT_GET(L, s, filename, lua_pushstring);
				STRUCT_GET(L, s, length_ms, lua_pushinteger);

				NEW_USER_OBJECT(L, Sound, &r.sounds.data[j]);
				lua_setfield(L, -2, "sound");

				lua_seti(L, -2, j + 1);
			}

			lua_setfield(L, -2, "sounds");
		}

		if (r.input_names.length != r.input_refs.length)
		{
			LOG_ERROR("input names and references number doesnt match: %d names to %d refs", r.input_names.length,
					  r.input_refs.length);
		}
		else if (r.input_names.length > 0)
		{
			lua_newtable(L);

			char *s;
			u32 j;
			vec_foreach(&r.input_names, s, j)
			{
				lua_newtable(L);
				// STRUCT_GET(L, s, filename, lua_pushstring);
				// STRUCT_GET(L, s, length_ms, lua_pushinteger);

				// NEW_USER_OBJECT(L, Sound, &r.sounds.data[j]);
				lua_pushstring(L, s);
				lua_setfield(L, -2, "name");
				lua_rawgeti(L, LUA_REGISTRYINDEX, r.input_refs.data[j]);
				lua_setfield(L, -2, "function");

				lua_seti(L, -2, j + 1);
			}

			lua_setfield(L, -2, "inputs");
		}

		lua_seti(L, -2, i + 1);
	}

	return 1;
}

static int lua_registry_register_block_input(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, BlockRegistry, wrapper, 1);
	u64 id = luaL_checkinteger(L, 2);
	const char *name = luaL_checkstring(L, 3);
	luaL_checkany(L, 4);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushboolean(L, scripting_register_block_input(wrapper->reg, id, ref, name) == SUCCESS);
	return 1;
}


void lua_block_registry_register(lua_State *L)
{
	const static luaL_Reg registry_methods[] = {
		{		 "get_name",			 lua_registry_get_name},
		{		 "to_table",			 lua_registry_to_table},
		{"register_input", lua_registry_register_block_input},
		{		  "uuid",				 lua_uuid_shared},
		{			NULL,							  NULL},
	};

	luaL_newmetatable(L, "BlockRegistry");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, registry_methods, 0);
}
