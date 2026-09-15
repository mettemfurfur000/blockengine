#include "include/general.h"
#include "include/scripting/level.h"

#include "include/scripting_bindings.h"

#include "include/block_registry.h"
#include "include/events.h"
#include "include/file_system.h"
#include "include/flags.h"
#include "include/handle.h"
#include "include/level.h"
#include "include/logging.h"
#include "include/scripting.h"
#include "include/scripting_var_handles.h"

#include "include/config.h"
#include "include/arena.h"

#include <box2d/box2d.h>
#include <box2d/math_functions.h>
#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>

#include "include/block_entity.h"

static int lua_level_create(lua_State *L)
{
	NEW_USER_OBJECT(L, Level, level_create(luaL_checkstring(L, 1)));
	return 1;
}

static int lua_load_level(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	LUA_CHECK_USER_OBJECT(L, BlockRegistry, RegWrapper, 2)
	level *out = calloc(1, sizeof(level));

	if (load_level_ack_registry(out, name, RegWrapper->reg) == SUCCESS)
	{
		NEW_USER_OBJECT(L, Level, out);
	}
	else
	{
		free(out);
		lua_pushnil(L);
	}

	return 1;
}

static int lua_save_level(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);

	lua_pushboolean(L, save_level(*wrapper->lvl) == SUCCESS);

	return 1;
}

static int lua_level_registry_serialize(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	const u32 index = luaL_checkinteger(L, 2);

	if (index >= wrapper->lvl->registries.length)
		luaL_error(L, "Index out of range");

	lua_pushboolean(L, registry_save(wrapper->lvl->registries.data[index]) == SUCCESS ? 1 : 0);
	return 1;
}

static int lua_level_registry_deserialize(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	const char *name = luaL_checkstring(L, 2);

	block_registry *r = registry_load(name);

	if (r)
	{
		(void)vec_push(&wrapper->lvl->registries, r);
		NEW_USER_OBJECT(L, BlockRegistry, r);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int lua_level_load_registry(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	const char *registry_name = luaL_checkstring(L, 2);

	block_registry *r = registry_load(registry_name);

	if (r != NULL)
	{
		(void)vec_push(&wrapper->lvl->registries, r);
		NEW_USER_OBJECT(L, BlockRegistry, r);
	}
	else
	{
		free(r);
		lua_pushnil(L);
	}

	return 1;
}

static int lua_level_get_registries(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);

	lua_newtable(L);

	vec_void_t *registries = &wrapper->lvl->registries;

	for (u32 i = 0; i < registries->length; i++)
	{
		NEW_USER_OBJECT(L, BlockRegistry, registries->data[i]);
		lua_seti(L, -2, i + 1);
	}

	return 1;
}

static int lua_level_add_existing(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
	LUA_CHECK_USER_OBJECT(L, BlockRegistry, reg_wrapper, 2);

	(void)vec_push(&wrapper->lvl->registries, reg_wrapper->reg);

	return 0;
}

static int lua_level_gc(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Level, wrapper, 1);
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
	u32 index = luaL_checkinteger(L, 2);

	if (index >= wrapper->lvl->rooms.length)
		luaL_error(L, "Index out of range");

	room *r = wrapper->lvl->rooms.data[index];

	NEW_USER_OBJECT(L, Room, r);

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

	for (u32 i = 0; i < wrapper->lvl->rooms.length; i++)
	{
		if (strcmp(((room *)wrapper->lvl->rooms.data[i])->name, name) == 0)
		{
			NEW_USER_OBJECT(L, Room, wrapper->lvl->rooms.data[i]);
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
	int w = luaL_checkinteger(L, 3);
	int h = luaL_checkinteger(L, 4);

	NEW_USER_OBJECT(L, Room, room_create(wrapper->lvl, name, w, h));
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

static int lua_room_box2d_tick(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);

	room *r = wrapper->r;

	const f32 timeStep = 1.0f / TPS;
	const u32 subStepCount = 100 / TPS;

	b2World_Step(r->b2_world_id, timeStep, subStepCount);

	return 0;
}

static int lua_room_new_layer(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
	const char *registry_name = luaL_checkstring(L, 2);

	int bytes_per_block = luaL_checkinteger(L, 3);
	int flags = luaL_checkinteger(L, 4);

	block_registry *reg = find_registry((((level *)wrapper->r->parent_level)->registries), (char *)registry_name);

	if (!reg)
		luaL_error(L, "Registry %s not found", registry_name);

	NEW_USER_OBJECT(L, Layer, layer_create(wrapper->r, reg, bytes_per_block, flags));

	return 1;
}

static int lua_room_get_layer(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
	u32 index = luaL_checkinteger(L, 2);

	if (index >= wrapper->r->layers.length)
		luaL_error(L, "Index out of range");

	NEW_USER_OBJECT(L, Layer, wrapper->r->layers.data[index]);
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

	if (x >= wrapper->l->width || y >= wrapper->l->height)
		luaL_error(L, "Coordinates out of range - %d,%d for layer size %d,%d", x, y, wrapper->l->width,
				   wrapper->l->height);

	if (!wrapper->l->registry)
		luaL_error(L, "Layer has no registry");

	if (id >= wrapper->l->registry->resources.length)
		luaL_error(L, "Block ID out of range - %d out of total %d blocks", id, wrapper->l->registry->resources.length);

	block_resources *res = &wrapper->l->registry->resources.data[id];

	u64 old_id = 0;

	if (id == 0)
	{
		if (block_delete_vars(wrapper->l, x, y) != SUCCESS)
			luaL_error(L, "Failed to delete vars at: %d, %d", x, y);
	}
	else
	{
		if (layer_copy_vars(wrapper->l, x, y, id == 0 ? (blob){} : res->vars_sample) != SUCCESS)
			luaL_error(L, "Failed to copy vars at: %d, %d", x, y);
	}

	if (block_get_id(wrapper->l, x, y, &old_id) != SUCCESS)
		luaL_error(L, "Failed to get id at: %d, %d", x, y);
	if (block_set_id(wrapper->l, x, y, id) != SUCCESS)
		luaL_error(L, "Failed to set id at: %d, %d", x, y);

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
	return 0;
}

static int lua_layer_move_block(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	i16 delta_x = (i16)luaL_checkinteger(L, 4);
	i16 delta_y = (i16)luaL_checkinteger(L, 5);

	lua_pushboolean(L, block_move(wrapper->l, x, y, delta_x, delta_y) == SUCCESS);

	return 1;
}

static bool lua_layer_id_in_list(lua_State *L, int index, u64 id);

#define FIND_PATH_IGNORE_SOURCE 0x01
#define FIND_PATH_IGNORE_TARGET 0x02

static int lua_layer_find_path(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	const i32 start_x = (i32)luaL_checkinteger(L, 2);
	const i32 start_y = (i32)luaL_checkinteger(L, 3);
	const i32 goal_x = (i32)luaL_checkinteger(L, 4);
	const i32 goal_y = (i32)luaL_checkinteger(L, 5);
	layer *blocked_layer = wrapper->l;

	if (!lua_isnoneornil(L, 6))
	{
		LUA_CHECK_USER_OBJECT(L, Layer, blocked_wrapper, 6);
		blocked_layer = blocked_wrapper->l;
	}
	const u32 path_flags = (u32)luaL_optinteger(L, 7, 0);
	const int ignored_ids_index = 8;
	const bool has_ignored_ids = !lua_isnoneornil(L, ignored_ids_index);
	if (has_ignored_ids)
		luaL_checktype(L, ignored_ids_index, LUA_TTABLE);

	const u32 width = blocked_layer->width;
	const u32 height = blocked_layer->height;
	const u32 node_count = width * height;

	if (start_x < 0 || start_y < 0 || goal_x < 0 || goal_y < 0 || (u32)start_x >= width ||
		(u32)start_y >= height || (u32)goal_x >= width || (u32)goal_y >= height)
	{
		lua_pushnil(L);
		return 1;
	}

	const u32 start = (u32)start_y * width + (u32)start_x;
	const u32 goal = (u32)goal_y * width + (u32)goal_x;
	u64 start_id = 0;
	u64 goal_id = 0;
	if (block_get_id(blocked_layer, (u16)start_x, (u16)start_y, &start_id) != SUCCESS ||
		block_get_id(blocked_layer, (u16)goal_x, (u16)goal_y, &goal_id) != SUCCESS)
	{
		lua_pushnil(L);
		return 1;
	}
	if (start_id != 0 && !(path_flags & FIND_PATH_IGNORE_SOURCE) &&
		(!has_ignored_ids || !lua_layer_id_in_list(L, ignored_ids_index, start_id)))
	{
		lua_pushnil(L);
		return 1;
	}
	if (goal_id != 0 && !(path_flags & FIND_PATH_IGNORE_TARGET) &&
		(!has_ignored_ids || !lua_layer_id_in_list(L, ignored_ids_index, goal_id)))
	{
		lua_pushnil(L);
		return 1;
	}
	static arena *path_arena = NULL;
	if (path_arena == NULL)
		path_arena = arena_create(8192);
	if (path_arena == NULL)
		return luaL_error(L, "find_path: failed to create search arena");
	arena_free(path_arena);

	u8 *visited = arena_alloc(path_arena, node_count * sizeof(u8));
	u32 *queue = arena_alloc(path_arena, node_count * sizeof(u32));
	i32 *parent = arena_alloc(path_arena, node_count * sizeof(i32));
	if (!visited || !queue || !parent)
	{
		arena_free(path_arena);
		return luaL_error(L, "find_path: failed to allocate search buffers");
	}

	memset(visited, 0, node_count * sizeof(u8));
	for (u32 i = 0; i < node_count; i++)
		parent[i] = -1;

	u32 queue_head = 0;
	u32 queue_tail = 0;
	queue[queue_tail++] = start;
	visited[start] = 1;

	static const i32 directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	while (queue_head < queue_tail && !visited[goal])
	{
		const u32 current = queue[queue_head++];
		const i32 current_x = (i32)(current % width);
		const i32 current_y = (i32)(current / width);

		for (u32 i = 0; i < 4; i++)
		{
			const i32 next_x = current_x + directions[i][0];
			const i32 next_y = current_y + directions[i][1];
			if (next_x < 0 || next_y < 0 || (u32)next_x >= width || (u32)next_y >= height)
				continue;

			const u32 next = (u32)next_y * width + (u32)next_x;
			if (visited[next])
				continue;

			u64 block_id = 0;
			if (block_get_id(blocked_layer, (u16)next_x, (u16)next_y, &block_id) != SUCCESS)
				continue;
			const bool ignore_endpoint = (next == start && (path_flags & FIND_PATH_IGNORE_SOURCE)) ||
				(next == goal && (path_flags & FIND_PATH_IGNORE_TARGET));
			if (block_id != 0 && !ignore_endpoint &&
				(!has_ignored_ids || !lua_layer_id_in_list(L, ignored_ids_index, block_id)))
				continue;

			visited[next] = 1;
			parent[next] = (i32)current;
			queue[queue_tail++] = next;
		}
	}

	if (!visited[goal])
	{
		arena_free(path_arena);
		lua_pushnil(L);
		return 1;
	}

	u32 path_length = 1;
	for (i32 node = (i32)goal; node != (i32)start; node = parent[node])
		path_length++;

	lua_newtable(L);
	u32 node = goal;
	for (u32 i = path_length; i > 0; i--)
	{
		lua_newtable(L);
		lua_pushinteger(L, (lua_Integer)(node % width));
		lua_setfield(L, -2, "x");
		lua_pushinteger(L, (lua_Integer)(node / width));
		lua_setfield(L, -2, "y");
		lua_seti(L, -2, (lua_Integer)i);
		node = (node == start) ? start : (u32)parent[node];
	}

	arena_free(path_arena);
	return 1;
}

static bool lua_layer_id_in_list(lua_State *L, int index, u64 id)
{
	luaL_checktype(L, index, LUA_TTABLE);
	const size_t length = lua_rawlen(L, index);
	for (size_t i = 1; i <= length; i++)
	{
		lua_rawgeti(L, index, (lua_Integer)i);
		const u64 candidate = (u64)luaL_checkinteger(L, -1);
		lua_pop(L, 1);
		if (candidate == id)
			return true;
	}
	return false;
}

static int lua_layer_find_closest(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);
	const i32 start_x = (i32)luaL_checkinteger(L, 2);
	const i32 start_y = (i32)luaL_checkinteger(L, 3);
	LUA_CHECK_USER_OBJECT(L, Layer, target_wrapper, 4);
	const int ids_index = 5;
	layer *blocked_layer = wrapper->l;
	bool has_exclusion = false;
	i32 exclude_x = 0;
	i32 exclude_y = 0;
	i32 exclude_radius = 0;
	if (!lua_isnoneornil(L, 6))
	{
		LUA_CHECK_USER_OBJECT(L, Layer, blocked_wrapper, 6);
		blocked_layer = blocked_wrapper->l;
	}
	if (!lua_isnoneornil(L, 7))
	{
		exclude_x = (i32)luaL_checkinteger(L, 7);
		exclude_y = (i32)luaL_checkinteger(L, 8);
		exclude_radius = (i32)luaL_checkinteger(L, 9);
		has_exclusion = true;
	}

	const u32 width = blocked_layer->width;
	const u32 height = blocked_layer->height;
	const u32 node_count = width * height;
	if (start_x < 0 || start_y < 0 || (u32)start_x >= width || (u32)start_y >= height)
	{
		lua_pushnil(L);
		return 1;
	}

	u8 *visited = calloc(node_count, sizeof(u8));
	u32 *queue = malloc(node_count * sizeof(u32));
	u32 *distance = calloc(node_count, sizeof(u32));
	if (!visited || !queue || !distance)
	{
		free(visited);
		free(queue);
		free(distance);
		return luaL_error(L, "find_closest: failed to allocate search buffers");
	}

	u32 queue_head = 0;
	u32 queue_tail = 0;
	const u32 start = (u32)start_y * width + (u32)start_x;
	queue[queue_tail++] = start;
	visited[start] = 1;
	static const i32 directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	i32 found_x = -1;
	i32 found_y = -1;

	while (queue_head < queue_tail)
	{
		const u32 current = queue[queue_head++];
		const i32 current_x = (i32)(current % width);
		const i32 current_y = (i32)(current / width);
		u64 target_id = 0;
		if (block_get_id(target_wrapper->l, (u16)current_x, (u16)current_y, &target_id) == SUCCESS &&
			lua_layer_id_in_list(L, ids_index, target_id) &&
			(!has_exclusion ||
				abs(current_x - exclude_x) + abs(current_y - exclude_y) > exclude_radius))
		{
			found_x = current_x;
			found_y = current_y;
			break;
		}

		for (u32 i = 0; i < 4; i++)
		{
			const i32 next_x = current_x + directions[i][0];
			const i32 next_y = current_y + directions[i][1];
			if (next_x < 0 || next_y < 0 || (u32)next_x >= width || (u32)next_y >= height)
				continue;
			const u32 next = (u32)next_y * width + (u32)next_x;
			if (visited[next])
				continue;
			u64 block_id = 0;
			if (block_get_id(blocked_layer, (u16)next_x, (u16)next_y, &block_id) != SUCCESS)
				continue;
			u64 next_target_id = 0;
			if (block_get_id(target_wrapper->l, (u16)next_x, (u16)next_y, &next_target_id) != SUCCESS)
				continue;
			const bool is_target = lua_layer_id_in_list(L, ids_index, next_target_id);
			if (block_id != 0 && !is_target)
				continue;
			visited[next] = 1;
			distance[next] = distance[current] + 1;
			queue[queue_tail++] = next;
		}
	}

	free(visited);
	free(queue);
	if (found_x < 0)
	{
		free(distance);
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);
	lua_pushinteger(L, found_x);
	lua_setfield(L, -2, "x");
	lua_pushinteger(L, found_y);
	lua_setfield(L, -2, "y");
	lua_pushinteger(L, (lua_Integer)distance[(u32)found_y * width + (u32)found_x]);
	lua_setfield(L, -2, "distance");
	free(distance);
	return 1;
}

static int lua_get_block_input_handler(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);

	const char *name = luaL_checkstring(L, 4);

	u64 id = 0;
	if (block_get_id(wrapper->l, x, y, &id) != SUCCESS)
	{
		lua_pushnil(L);
		return 1;
	}

	vec_int_t *refs = &wrapper->l->registry->resources.data[id].input_refs;
	vec_str_t *ref_names = &wrapper->l->registry->resources.data[id].input_names;

	for (u32 i = 0; i < refs->length; i++)
		if (strcmp(ref_names->data[i], name) == 0)
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, refs->data[i]);
			return 1;
		}

	lua_pushnil(L);
	return 1;
}

static int lua_layer_get_size(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	lua_pushinteger(L, wrapper->l->width);
	lua_pushinteger(L, wrapper->l->height);
	lua_pushinteger(L, wrapper->l->block_size);
	// lua_pushinteger(L, wrapper->sizeof(handle32));

	return 3;
}

static int lua_layer_set_static(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);
	bool val = luaL_checkinteger(L, 2);

	FLAG_SET(wrapper->l->flags, LAYER_FLAG_STATIC, val);
	return 0;
}

static int lua_layer_for_each(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u64 filter = luaL_checkinteger(L, 2); // a block id that is beink searched (?)
	luaL_checktype(L, 3, LUA_TFUNCTION);  // callback

	u32 w = wrapper->l->width;
	u32 h = wrapper->l->height;

	u64 id;

	for (u32 j = 0; j < h; j++)
		for (u32 i = 0; i < w; i++)
		{
			if (block_get_id(wrapper->l, i, j, &id) != SUCCESS)
			{
				LOG_ERROR("Error getting a block at %d %d");
				return 0;
			}

			// LOG_DEBUG("foreach found %d agains filter id %d", id, filter);

			if (id == filter)
			{
				lua_pushvalue(L, 3);
				lua_pushinteger(L, i);
				lua_pushinteger(L, j);

				if (lua_pcall(g_L, 2, 0, 0) != 0)
				{
					LOG_ERROR("Error calling a callback: %s", lua_tostring(g_L, -1));
					lua_pop(g_L, 1);
					return 0;
				}
			}
		}

	return 0;
}

static int lua_layer_set_id(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	u64 id = luaL_checknumber(L, 4);

	if (block_set_id(wrapper->l, x, y, id) == SUCCESS)
	{
		/* Push block update to layer accumulator */
		// push_block_update(&wrapper->l->id_updates, x, y, id, wrapper->l->block_size);
		lua_pushboolean(L, 1);
	}
	else
	{
		lua_pushboolean(L, 0);
	}

	return 1;
}

static int lua_block_get_id(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	u64 id = 0;

	if (block_get_id(wrapper->l, x, y, &id) == SUCCESS)
		lua_pushnumber(L, id);
	else
		lua_pushnil(L);

	return 1;
}

static int lua_block_get_vars(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	/* Return a VarHandle instead of raw Vars pointer. First return boolean success, then handle or nil */
	handle32 h = block_get_var_handle(wrapper->l, x, y);

	if (!handle_is_valid(wrapper->l->var_pool.table, h)) // not valid nuh uh
		lua_pushnil(L);
	else
		push_varhandle(L, wrapper->l, h);

	return 1;
}

static int lua_block_copy_vars(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u32 x = luaL_checknumber(L, 2);
	u32 y = luaL_checknumber(L, 3);
	/* Accept either a Vars userdata (blob pointer) or a VarHandle userdata */
	blob *src_blob = NULL;

	if (luaL_testudata(L, 4, "VarHandle"))
	{
		src_blob = get_blob_from_varhandle(L, 4);
		if (!src_blob)
		{
			lua_pushboolean(L, 0);
			return 1;
		}
	}
	else
	{
		luaL_error(L, "Expected VarHandle as 4th argument");
	}

	lua_pushboolean(L, layer_copy_vars(wrapper->l, x, y, *src_blob) == SUCCESS);
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

typedef struct
{
	layer *l;
	f32 dt;
	lua_State *L;
	u64 value;
} tick_iter_data;

u32 block_entity_tick_script(lua_State *L, layer *l, block_entity *e, u64 value)
{
	u64 id = e->block_id;

	if (id == 0 || id >= l->registry->resources.length)
		return SUCCESS; // air and invalid blocks dont need anything

	i32 ref = l->registry->resources.data[id].entity_tick_ref;
	if (ref == 0)
		return SUCCESS; // no ref means block has no tick logic

	assert(lua_rawgeti(L, LUA_REGISTRYINDEX, ref) == LUA_TFUNCTION); // ref doesnt point to a function...

	lua_pushvalue(L, 1); // still pushes that layer value to the ticker function
	NEW_USER_OBJECT_HANDLE32(L, BlockEntity, l, e->handle);
	lua_pushinteger(L, value);

	if (lua_pcall(L, 3, 0, 0) != 0)
	{
		LOG_ERROR("Error calling an entity tick callback: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_pushnil(L);
		return FAIL;
	}

	return SUCCESS;
}

u32 block_entity_tick_each(handle32 h, void *ptr, void *user_data)
{
	block_entity *e = (block_entity *)ptr;
	tick_iter_data *info = user_data;
	assert(e);

	layer *l = e->parent_layer;
	assert(l);

	if (!b2Body_IsValid(e->b2_body_id))
		return SUCCESS;

	b2Transform t = b2Body_GetTransform(e->b2_body_id);

	f32 rotation = b2Rot_GetAngle(t.q);

	LOG_DEBUG("entity %x, pos(%.2f,%.2f), rot %f", h, t.p.x, t.p.y, rotation);

	e->pos_old.x = t.p.x;
	e->pos_old.y = t.p.y;

	e->timestamp_old = SDL_GetTicks();

	if (!handle_is_valid(info->l->block_entity_pool, h))
		return SUCCESS;

	if (block_entity_tick_script(info->L, info->l, e, info->value) != SUCCESS)
		return FAIL;

	if (!handle_is_valid(info->l->block_entity_pool, h))
		return SUCCESS;

	return SUCCESS;
}

void layer_tick_entities(lua_State *L, layer *l, float dt, u64 value)
{
	tick_iter_data info = {.dt = dt, .l = l, .L = L, .value = value};

	l->block_entity_count_estimate = handle_table_iterate(l->block_entity_pool, block_entity_tick_each, &info);
}

static int lua_layer_tick_blocks(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);
	const u64 value = luaL_checkinteger(L, 2);

	const u32 w = wrapper->l->width;
	const u32 h = wrapper->l->height;

	u64 id;

	int ref = 0;

	const block_registry *reg = wrapper->l->registry;

	for (u32 j = 0; j < h; j++)
		for (u32 i = 0; i < w; i++)
		{
			if (block_get_id(wrapper->l, i, j, &id) != SUCCESS)
			{
				LOG_ERROR("tick errored for block %d:%d on layer %lld", i, j, wrapper->l->uuid);
				lua_pushnil(L);
				return 1;
			}

			if (id == 0 || id >= wrapper->l->registry->resources.length) // ignore invalid values or empty blocks
				continue;

			ref = reg->resources.data[id].input_tick_ref; // get the tick function from the registry
			if (ref == 0)
				continue;

			if (lua_rawgeti(L, LUA_REGISTRYINDEX, ref) == LUA_TFUNCTION) // execute it
			{
				lua_pushvalue(L, 1);
				lua_pushinteger(L, i);
				lua_pushinteger(L, j);
				lua_pushinteger(L, value);

				if (lua_pcall(L, 4, 0, 0) != 0)
				{
					LOG_ERROR("Error calling a tick callback: %s", lua_tostring(L, -1));
					lua_pop(L, 1);
					lua_pushnil(L);
					return 1;
				}
			}
		}

	if (wrapper->l->block_entity_pool)
		layer_tick_entities(L, wrapper->l, 1.0f / TPS, value);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_layer_new_entity(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u64 block_id = (u64)luaL_checkinteger(L, 2);
	float x = (float)luaL_checknumber(L, 3);
	float y = (float)luaL_checknumber(L, 4);

	handle32 h = layer_add_block_entity(wrapper->l, block_id, x, y);
	if (!handle_is_valid(wrapper->l->block_entity_pool, h))
	{
		luaL_error(L, "Failed to create block entity on layer %lld", wrapper->l->uuid);
		lua_pushnil(L);
		return 1;
	}

	NEW_USER_OBJECT_HANDLE32(L, BlockEntity, wrapper->l, h);
	return 1;
}

u32 iter_entities_get(handle32 h, void *ptr, void *user_data)
{
	void **inputs = (void **)user_data;
	lua_State *L = (lua_State *)inputs[0];
	layer *l = (layer *)inputs[1];
	block_entity *e = (block_entity *)ptr;

	assert(e);

	handle32convertor c = {.h = h};
	lua_pushinteger(L, c.i);
	NEW_USER_OBJECT_HANDLE32(L, BlockEntity, l, h);
	lua_settable(L, -3);
	return SUCCESS;
}

static int lua_layer_get_entities(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	if (!wrapper->l->block_entity_pool)
	{
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);

	void *pointers[] = {(void *)L, (void *)wrapper->l};

	handle_table_iterate(wrapper->l->block_entity_pool, iter_entities_get, pointers);

	return 1;
}

static int lua_layer_cleanup_unused_vars(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	u8 result = layer_cleanup_unused_vars(wrapper->l);

	lua_pushboolean(L, result == SUCCESS);
	return 1;
}

static int lua_layer_build_ground_physics(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Layer, wrapper, 1);

	layer_build_ground_physics(wrapper->l);

	return 0;
}

void lua_layer_register(lua_State *L)
{
	const static luaL_Reg layer_methods[] = {
		{			"get_size",			 lua_layer_get_size},
		{			"for_each",			 lua_layer_for_each},
		{			  "set_id",			   lua_layer_set_id},
		{			  "get_id",			   lua_block_get_id},
		{		  "move_block",		   lua_layer_move_block},
		{		  "find_path",		   lua_layer_find_path},
		{		"find_closest",		 lua_layer_find_closest},
		{		 "paste_block",			lua_layer_paste_block},
		{	 "get_input_handler",	  lua_get_block_input_handler},
		{		  "set_static",		   lua_layer_set_static},
		{			"get_vars",			 lua_block_get_vars},
		{			"set_vars",			 lua_block_copy_vars},
		{			  "bprint",					lua_bprintf},
		{				"tick",			 lua_layer_tick_blocks},
		{				"uuid",				 lua_uuid_shared},
		{		  "new_entity",		   lua_layer_new_entity},
		{		 "get_entities",		 lua_layer_get_entities},
		{ "cleanup_unused_vars",	lua_layer_cleanup_unused_vars},
		{"build_ground_physics", lua_layer_build_ground_physics},
		{				  NULL,						   NULL},
	};

	luaL_newmetatable(L, "Layer");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, layer_methods, 0);
}

void lua_level_register(lua_State *L)
{
	const static luaL_Reg level_methods[] = {
		{		 "load_registry",		  lua_level_load_registry},
		{	 "serialize_registry",   lua_level_registry_serialize},
		{"deserialize_registry", lua_level_registry_deserialize},
		{		 "get_registries",	   lua_level_get_registries},
		{		 "add_existing",		 lua_level_add_existing},
		{		 "get_room_count",	   lua_level_get_room_count},
		{		   "find_room",	  lua_level_get_room_by_name},
		{			"get_name",			 lua_level_get_name},
		{			"get_room",			 lua_level_get_room},
		{			"new_room",			 lua_level_new_room},
		{				"uuid",				 lua_uuid_shared},
		{				"__gc",				   lua_level_gc},
		{				  NULL,						   NULL},
	};

	luaL_newmetatable(L, "Level");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, level_methods, 0);
}

void lua_room_register(lua_State *L)
{
	const static luaL_Reg room_methods[] = {
		{		 "get_name",		 lua_room_get_name},
		   {		"get_size",		lua_room_get_size},
		{		 "get_layer",		  lua_room_get_layer},
		   {"get_layer_count", lua_room_get_layer_count},
		{		 "new_layer",		  lua_room_new_layer},
		   {		"box2d_tick",	  lua_room_box2d_tick},
		{		   "uuid",			 lua_uuid_shared},
		   {				NULL,					  NULL},
	};

	luaL_newmetatable(L, "Room");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, room_methods, 0);
}

void lua_level_editing_lib_register(lua_State *L)
{
	const static luaL_Reg level_editing_methods[] = {
		{"create_level", lua_level_create},
		{	 "load_level",   lua_load_level},
		{	 "save_level",   lua_save_level},
		{		  NULL,			   NULL},
	};

	luaL_newlib(L, level_editing_methods);
	lua_setglobal(L, "le");
}