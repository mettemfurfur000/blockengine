#ifndef BLOCK_RENDERER_V2_H
#define BLOCK_RENDERER_V2_H

#include "general.h"
#include "image_editing.h"

#include <epoxy/gl.h>

typedef struct
{
	float x, y;
	float scale_x, scale_y;
	float rotation;
	u8 frame;
	u8 type;
	u8 flags;
	u8 padding;
} instance_data;

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	GLuint instance_vbo;
	GLuint shader;
	GLint projection_loc;
	GLint resize_ratio_loc;
	GLint block_width_loc;
	GLint texture_loc;
	GLint color_loc;
	GLint use_color_loc;
} shader_program;

typedef struct
{
	instance_data *data;
	u32 capacity;
	u32 count;
	GLuint texture;
	image *atlas_img;
	u8 block_width;
} layer_batch;

typedef struct
{
	shader_program standard;
	shader_program post;
	GLuint post_fbo;
	GLuint post_texture;
	GLuint post_vao;
	GLuint post_vbo;
	GLuint post_ebo;
	layer_batch batch;
	bool initialized;
} block_renderer_v2;

extern block_renderer_v2 renderer_v2;

int renderer_v2_init(void);
void renderer_v2_shutdown(void);

void renderer_v2_begin_batch(GLuint texture, image *atlas_img, u8 block_width);
int renderer_v2_add_instance(float x, float y, u8 frame, u8 type, u8 flags, float scale_x, float scale_y,
							 float rotation);
void renderer_v2_end_batch(void);

void renderer_v2_begin_frame(void);
void renderer_v2_end_frame(void);
void renderer_v2_resize(u16 width, u16 height);

#endif
