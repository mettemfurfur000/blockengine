#ifndef OPENGL_STUFF_H
#define OPENGL_STUFF_H

#include "general.h"
#include <epoxy/gl.h>
#include <SDL2/SDL_opengl.h>

// for stuff that only requires opengl and not sdl2

void setup_opengl(u16 width, u16 height);

u32 load_shader(const char *shader_path, GLenum shader_type);
u32 compile_shader_program(u32 *shaders, u8 len);
u32 assemble_shader(const char *shader_name);

#endif