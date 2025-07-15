#ifndef OPENGL_STUFF_H
#define OPENGL_STUFF_H

#include "general.h"
#include <epoxy/gl.h>
#include <SDL2/SDL_opengl.h>

#include "image_editing.h"

// for stuff that mostly requires opengl 

void setup_opengl(u16 width, u16 height);

u32 load_shader(const char *shader_path, GLenum shader_type);
u32 compile_shader_program(u32 *shaders, u8 len);
u32 assemble_shader(const char *shader_name);

GLuint gl_bind_texture(image *src);

#endif