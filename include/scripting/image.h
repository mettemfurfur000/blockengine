#ifndef SCRIPTING_IMAGE_H
#define SCRIPTING_IMAGE_H

#include "include/image_editing.h"
#include <lua.h>

#define PUSHNEWIMAGE(L, i, name)                                                                                       \
	ImageWrapper *name = (ImageWrapper *)lua_newuserdata(L, sizeof(ImageWrapper));                                     \
	name->img = (i);                                                                                                   \
	luaL_getmetatable(L, "Image");                                                                                     \
	lua_setmetatable(L, -2);

typedef struct
{
	image *img;
} ImageWrapper;

void image_load_editing_library(lua_State *L);

#endif