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
	float tile_u;
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

	GLuint instance_vbo;
	GLuint vao;
	u32 vbo_capacity;
} layer_batch;

typedef struct
{
	shader_program standard;
	layer_batch batch;
	GLuint dummy_texture;
	bool initialized;

	f32 view_cam_x, view_cam_y;
	f32 view_zoom;
	u16 view_w, view_h;
} block_renderer_v2;

extern block_renderer_v2 renderer_v2;

int renderer_v2_init(void);
void renderer_v2_shutdown(void);

void renderer_v2_begin_batch(GLuint texture, image *atlas_img, u8 block_width);
int renderer_v2_add_instance(float x, float y, u8 frame, u8 type, u8 flags, float scale_x, float scale_y,
							 float rotation);
void renderer_v2_end_batch(void);

void renderer_v2_resize(u16 width, u16 height);

// Stores the current camera/viewport and rebuilds the projection so that a
// world pixel W maps to screen (W * zoom - cam). cam_x/cam_y are in scaled
// space (screen pixels). Cheap to call per frame.
void renderer_v2_set_view(u16 width, u16 height, f32 cam_x, f32 cam_y, f32 zoom);

// Updates only the projection matrix to match a (possibly different) viewport.
// Does not recreate the post-processing framebuffer, so it is cheap to call per frame.
void renderer_v2_set_projection_size(u16 width, u16 height);

// Per-layer instance buffer helpers. Each layer owns its own VBO/VAO so slot
// ranges can be updated in place and drawn from offset 0 without a full
// re-upload every frame.
void renderer_v2_layer_begin(layer_batch *batch, GLuint texture, image *atlas_img, u8 block_width);
void renderer_v2_layer_upload(layer_batch *batch, u32 offset, u32 count, const instance_data *src);

// Ensures the instance buffer can hold `needed` instances. Returns true when
// the underlying GPU buffer was (re)created this call, which invalidates any
// previously uploaded data.
bool renderer_v2_layer_reserve(layer_batch *batch, u32 needed);
void renderer_v2_layer_draw(const layer_batch *batch, u32 count);

// Releases the per-layer GPU resources.
void renderer_v2_layer_release(layer_batch *batch);

// Draws a plain 1:1 textured quad over world rect [x, x+w] x [y, y+h] using the
// standard program with the texture covering the whole quad. Used to composite
// pre-rendered layer framebuffers. Does not go through the instance batch API.
void renderer_v2_draw_texture_quad(GLuint texture, f32 x, f32 y, f32 w, f32 h);

// Draws a single solid colored rectangle in screen pixels using the standard shader.
// Used by the room/camera pipeline for backgrounds and debug overlays.
void renderer_v2_fill_rect(f32 x, f32 y, f32 w, f32 h, const f32 color[4]);

#endif
