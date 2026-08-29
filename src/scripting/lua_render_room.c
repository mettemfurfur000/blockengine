#include "include/scripting/render_room.h"

#include "include/block_entity.h"
#include "include/logging.h"
#include "include/render_room.h"
#include "include/scripting.h"

#include <box2d/box2d.h>

static camera *lua_check_camera(lua_State *L, int index)
{
	LUA_CHECK_USER_OBJECT(L, Camera, cw, index);
	return (camera *)cw->ptr;
}

static int lua_rr_create_camera(lua_State *L)
{
	int w = luaL_checkinteger(L, 1);
	int h = luaL_checkinteger(L, 2);
	f32 zoom = (f32)luaL_optnumber(L, 3, 1.0);

	if (w <= 0 || h <= 0)
		luaL_error(L, "create_camera: viewport width/height must be positive");

	camera *cam = (camera *)calloc(1, sizeof(camera));
	camera_init(cam, (u16)w, (u16)h, zoom);

	NEW_USER_OBJECT(L, Camera, cam);
	return 1;
}

static int lua_rr_activate(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Room, rw, 1);
	LUA_CHECK_USER_OBJECT(L, Camera, cw, 2);

	render_room_activate(rw->r, (camera *)cw->ptr);
	return 0;
}

static int lua_rr_deactivate(lua_State *L)
{
	render_room_deactivate();
	return 0;
}

static int lua_rr_set_options(lua_State *L)
{
	room_render_options o = {0};
	o.clear_background = true;
	o.background_color[0] = 0.08f;
	o.background_color[1] = 0.08f;
	o.background_color[2] = 0.12f;
	o.background_color[3] = 1.0f;
	o.draw_grid = false;
	o.grid_color[0] = 0.30f;
	o.grid_color[1] = 0.30f;
	o.grid_color[2] = 0.30f;
	o.grid_color[3] = 0.50f;

	if (lua_istable(L, 1))
	{
		lua_getfield(L, 1, "clear_background");
		if (lua_isboolean(L, -1))
			o.clear_background = lua_toboolean(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "background_color");
		if (lua_istable(L, -1))
		{
			for (int i = 0; i < 4; i++)
			{
				lua_geti(L, -1, i + 1);
				if (lua_isnumber(L, -1))
					o.background_color[i] = (f32)lua_tonumber(L, -1);
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);

		lua_getfield(L, 1, "draw_grid");
		if (lua_isboolean(L, -1))
			o.draw_grid = lua_toboolean(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "grid_color");
		if (lua_istable(L, -1))
		{
			for (int i = 0; i < 4; i++)
			{
				lua_geti(L, -1, i + 1);
				if (lua_isnumber(L, -1))
					o.grid_color[i] = (f32)lua_tonumber(L, -1);
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
	}

	render_room_set_options(&o);
	return 0;
}

static int lua_cam_set_zoom(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	f32 z = (f32)luaL_checknumber(L, 2);
	if (z < 0.1f)
		z = 0.1f;
	cam->zoom = z;
	return 0;
}

static int lua_cam_get_zoom(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	lua_pushnumber(L, cam->zoom);
	return 1;
}

static int lua_cam_center_on(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	f32 x = (f32)luaL_checknumber(L, 2);
	f32 y = (f32)luaL_checknumber(L, 3);
	camera_center_on(cam, x, y);
	return 0;
}

static int lua_cam_set_position(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	cam->target_x = (f32)luaL_checknumber(L, 2);
	cam->target_y = (f32)luaL_checknumber(L, 3);
	return 0;
}

static int lua_cam_set_follow(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);

	if (lua_isnoneornil(L, 2))
	{
		camera_set_follow(cam, NULL, INVALID_HANDLE);
		return 0;
	}

	LUA_CHECK_USER_OBJECT(L, BlockEntity, ew, 2);
	camera_set_follow(cam, ew->l, ew->h);
	return 0;
}

static int lua_cam_clear_follow(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	camera_set_follow(cam, NULL, INVALID_HANDLE);
	return 0;
}

static int lua_cam_set_follow_block(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	LUA_CHECK_USER_OBJECT(L, Layer, lw, 2);
	u32 bx = (u32)luaL_checkinteger(L, 3);
	u32 by = (u32)luaL_checkinteger(L, 4);
	camera_set_follow_block(cam, lw->l, bx, by);
	return 0;
}

static int lua_cam_get_position(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	f32 dx = 0.0f, dy = 0.0f;
	camera_get_displayed_position(cam, &dx, &dy);
	lua_newtable(L);
	lua_pushnumber(L, dx);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, dy);
	lua_setfield(L, -2, "y");
	return 1;
}

static int lua_cam_set_viewport(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	cam->viewport_w = (u16)luaL_checkinteger(L, 2);
	cam->viewport_h = (u16)luaL_checkinteger(L, 3);
	return 0;
}

static int lua_cam_update(lua_State *L)
{
	camera *cam = lua_check_camera(L, 1);
	camera_update(cam);
	return 0;
}

static int lua_cam_gc(lua_State *L)
{
	LUA_CHECK_USER_OBJECT(L, Camera, cw, 1);
	camera *cam = (camera *)cw->ptr;
	if (cam)
	{
		if (render_room_active_camera == cam)
			render_room_deactivate();
		free(cam);
		cw->ptr = NULL;
	}
	return 0;
}

void lua_render_room_register(lua_State *L)
{
	static const luaL_Reg rr_lib[] = {
		{"create_camera", lua_rr_create_camera},
		{"activate", lua_rr_activate},
		{"deactivate", lua_rr_deactivate},
		{"set_options", lua_rr_set_options},
		{NULL, NULL},
	};

	luaL_newlib(L, rr_lib);
	lua_setglobal(L, "render_room");

	static const luaL_Reg cam_methods[] = {
		{"set_zoom", lua_cam_set_zoom},
		{"get_zoom", lua_cam_get_zoom},
		{"center_on", lua_cam_center_on},
		{"set_position", lua_cam_set_position},
		{"set_follow", lua_cam_set_follow},
		{"set_follow_block", lua_cam_set_follow_block},
		{"clear_follow", lua_cam_clear_follow},
		{"get_position", lua_cam_get_position},
		{"set_viewport", lua_cam_set_viewport},
		{"update", lua_cam_update},
		{"__gc", lua_cam_gc},
		{NULL, NULL},
	};

	luaL_newmetatable(L, "Camera");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, cam_methods, 0);
}
