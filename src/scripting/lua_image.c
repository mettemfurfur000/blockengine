#include "include/scripting/image.h"

#include <lauxlib.h>
#include <lua.h>

static int image_gc(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	if (wrapper->img)
	{
		image_free(wrapper->img);
		wrapper->img = NULL;
	}

	return 0;
}

static int image_create_lua(lua_State *L)
{
	u32 width = luaL_checkinteger(L, 1);
	u32 height = luaL_checkinteger(L, 2);

	PUSHNEWIMAGE(L, image_create(width, height), wrapper);
	return 1;
}

static int image_size_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	lua_pushinteger(L, wrapper->img->width);
	lua_pushinteger(L, wrapper->img->height);

	return 2;
}

static int image_clear_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	image_clear(wrapper->img);

	lua_pushvalue(L, 1);
	return 1;
}

static int image_adjust_brightness_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	float brightness = luaL_checknumber(L, 2);

	image_adjust_brightness(wrapper->img, brightness);

	lua_pushvalue(L, 1);
	return 1;
}

static int image_apply_color_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u8 color[4] = {
		luaL_checkinteger(L, 2), // r
		luaL_checkinteger(L, 3), // g
		luaL_checkinteger(L, 4), // b
		luaL_checkinteger(L, 5)	 // a
	};

	image_apply_color(wrapper->img, color);

	lua_pushvalue(L, 1);
	return 1;
}

static int get_average_color(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u8 color_out[4] = {};

	image_get_avg_color(wrapper->img, color_out);

	lua_pushinteger(L, color_out[0]);
	lua_pushinteger(L, color_out[1]);
	lua_pushinteger(L, color_out[2]);
	lua_pushinteger(L, 255);

	return 4;
}

static int image_gamma_correction_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");
	float gamma = luaL_checknumber(L, 2);

	image_gamma_correction(wrapper->img, gamma);

	lua_pushvalue(L, 1);

	return 1;
}

static int image_fill_color_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u8 color[4] = {luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5)};

	image_fill_color(wrapper->img, color);

	lua_pushvalue(L, 1);
	return 1;
}

static int image_overlay_lua(lua_State *L)
{
	ImageWrapper *dst = (ImageWrapper *)luaL_checkudata(L, 1, "Image");
	ImageWrapper *src = (ImageWrapper *)luaL_checkudata(L, 2, "Image");

	u32 x = luaL_checkinteger(L, 3);
	u32 y = luaL_checkinteger(L, 4);

	image_overlay(dst->img, src->img, x, y);

	lua_pushvalue(L, 1);
	return 1;
}

static int image_rotate_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u8 angle = luaL_checkinteger(L, 2);

	PUSHNEWIMAGE(L, image_rotate(wrapper->img, angle), result);

	return 1;
}

static int image_flip_horizontal_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	PUSHNEWIMAGE(L, image_flip_horizontal(wrapper->img), result);

	return 1;
}

static int image_flip_vertical_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	PUSHNEWIMAGE(L, image_flip_vertical(wrapper->img), result);

	return 1;
}

static int image_copy_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	PUSHNEWIMAGE(L, image_copy(wrapper->img), result);

	return 1;
}

static int image_save_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	const char *path = luaL_checkstring(L, 2);
	image_save(wrapper->img, path);

	return 0;
}

static int image_load_lua(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);

	image *i = image_load(path);

	if (i != NULL)
	{
		PUSHNEWIMAGE(L, i, result);
	}
	else
		lua_pushnil(L);
	return 1;
}

// geometry

static int image_crop_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u32 x = luaL_checkinteger(L, 2);
	u32 y = luaL_checkinteger(L, 3);
	u32 width = luaL_checkinteger(L, 4);
	u32 height = luaL_checkinteger(L, 5);

	PUSHNEWIMAGE(L, image_crop(wrapper->img, x, y, width, height), result);
	return 1;
}

// direct access

static int image_get_pixel(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u32 x = luaL_checkinteger(L, 2);
	u32 y = luaL_checkinteger(L, 3);

	lua_pushinteger(L, *ACCESS_CHANNEL(wrapper->img, x, y, 0));
	lua_pushinteger(L, *ACCESS_CHANNEL(wrapper->img, x, y, 1));
	lua_pushinteger(L, *ACCESS_CHANNEL(wrapper->img, x, y, 2));
	lua_pushinteger(L, *ACCESS_CHANNEL(wrapper->img, x, y, 3));

	return 4;
}

static int image_set_pixel(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u32 x = luaL_checkinteger(L, 2);
	u32 y = luaL_checkinteger(L, 3);

	*ACCESS_CHANNEL(wrapper->img, x, y, 0) = (luaL_checkinteger(L, 4));
	*ACCESS_CHANNEL(wrapper->img, x, y, 1) = (luaL_checkinteger(L, 5));
	*ACCESS_CHANNEL(wrapper->img, x, y, 2) = (luaL_checkinteger(L, 6));
	*ACCESS_CHANNEL(wrapper->img, x, y, 3) = (luaL_checkinteger(L, 7));

	lua_pushvalue(L, 1);
	return 1;
}

// conversion

static int image_to_bytes_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	size_t out_size = wrapper->img->width * wrapper->img->height * CHANNELS;
	u8 *bytes = wrapper->img->data;

	lua_pushlstring(L, (const char *)bytes, out_size);
	return 1;
}

static int image_from_bytes_lua(lua_State *L)
{
	size_t bytes_size;
	const char *bytes = luaL_checklstring(L, 1, &bytes_size);

	u16 width = luaL_checkinteger(L, 2);
	u16 height = luaL_checkinteger(L, 3);

	image *img = image_create(width, height);
	memcpy(img->data, bytes, bytes_size);

	PUSHNEWIMAGE(L, img, result);
	return 1;
}

static int image_quantize_n_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u8 n = luaL_checkinteger(L, 2);

	image_quantize_n(wrapper->img, n);

	return 0;
}

static int image_dither_floyd_steinberg_lua(lua_State *L)
{
	ImageWrapper *wrapper = (ImageWrapper *)luaL_checkudata(L, 1, "Image");

	u8 n = luaL_checkinteger(L, 2);

	image_dither_floyd_steinberg(wrapper->img, n);

	lua_pushvalue(L, 1);
	return 0;
}

void image_load_editing_library(lua_State *L)
{
	const static luaL_Reg image_methods[] = {
		{				  "crop",				   image_crop_lua},
		{				"rotate",				   image_rotate_lua},
		{		 "flip_horizontal",		image_flip_horizontal_lua},
		{		 "flip_vertical",		  image_flip_vertical_lua},

		{		 "add_brightness",	   image_adjust_brightness_lua},
		{			 "add_color",			  image_apply_color_lua},
		{		 "get_avg_color",				  get_average_color},
		{"image_gamma_correction",	   image_gamma_correction_lua},

		//{"blend", blend_images_lua},
		{				  "size",				   image_size_lua},
		{			   "overlay",				image_overlay_lua},

		{				 "clear",				  image_clear_lua},
		{				  "fill",			 image_fill_color_lua},
		{				  "copy",				   image_copy_lua},

		{			 "get_pixel",				  image_get_pixel},
		{			 "set_pixel",				  image_set_pixel},

		{				  "save",				   image_save_lua},
		{			  "to_bytes",				 image_to_bytes_lua},
		{			  "quantize",			 image_quantize_n_lua},
		{				"dither", image_dither_floyd_steinberg_lua},

		{					NULL,							 NULL}
	};

	// Create metatable for Image userdata
	luaL_newmetatable(L, "Image");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, image_methods, 0);
	lua_pushcfunction(L, image_gc);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);

	const static luaL_Reg image_editing_lib[] = {
		{		   "create",	   image_create_lua},
		{			 "load",		 image_load_lua},
		{"create_from_bytes", image_from_bytes_lua},
		{			   NULL,				 NULL}
	};

	// Create library
	luaL_newlib(L, image_editing_lib);
	lua_setglobal(L, "ie");
}
