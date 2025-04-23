#ifndef OPENGL_STUFF_H
#define OPENGL_STUFF_H

#include "general.h"
#include <epoxy/gl.h>
#include <SDL2/SDL_opengl.h>

// for stuff that only requires opengl and not sdl2

void setup_opengl(u16 width, u16 height);

#endif