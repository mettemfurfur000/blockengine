#include "include/block_renderer_v2.h"

#include "include/folder_structure.h"
#include "include/logging.h"
#include "include/opengl_stuff.h"
#include "include/sdl2_basics.h"

#include "include/config.h"

#include <epoxy/gl_generated.h>

#include <stdlib.h>
#include <string.h>

block_renderer_v2 renderer_v2 = {0};

static image g_dummy_img = {.width = 1, .height = 1, .data = NULL};

// Column-major orthographic projection: screen (world*zoom - cam) -> NDC.
static void renderer_v2_fill_projection(float p[16], u16 width, u16 height, f32 cam_x, f32 cam_y, f32 zoom)
{
	memset(p, 0, 16 * sizeof(float));
	p[0] = 2.0f * zoom / width;
	p[5] = -2.0f * zoom / height;
	p[10] = -1.0f;
	p[12] = -(2.0f * cam_x) / width - 1.0f;
	p[13] = (2.0f * cam_y) / height + 1.0f;
	p[15] = 1.0f;
}

static const float *renderer_v2_projection_matrix(u16 width, u16 height, f32 cam_x, f32 cam_y, f32 zoom)
{
	static float p[16];
	renderer_v2_fill_projection(p, width, height, cam_x, cam_y, zoom);
	return p;
}

static const float quad_vertices[] = {
	0.0f, 0.0f, 0.0f, 0.0f, //
	1.0f, 0.0f, 1.0f, 0.0f, //
	1.0f, 1.0f, 1.0f, 1.0f, //
	0.0f, 1.0f, 0.0f, 1.0f	//
};

static const unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

static GLuint compile_shader_program_v2(const char *name)
{
	char path[MAX_PATH_LENGTH];
	GLuint shaders[3];
	u8 shader_count = 0;

	snprintf(path, sizeof(path), FOLDER_SHD SEPARATOR_STR "%s." FOLDER_SHD_VERT_EXT, name);
	shaders[shader_count++] = load_shader(path, GL_VERTEX_SHADER);
	if (shaders[0] == 0)
		return 0;

	snprintf(path, sizeof(path), FOLDER_SHD SEPARATOR_STR "%s." FOLDER_SHD_FRAG_EXT, name);
	shaders[shader_count++] = load_shader(path, GL_FRAGMENT_SHADER);
	if (shaders[1] == 0)
		return 0;

	snprintf(path, sizeof(path), FOLDER_SHD SEPARATOR_STR "%s." FOLDER_SHD_GEOM_EXT, name);
	shaders[2] = load_shader(path, GL_GEOMETRY_SHADER);

	return compile_shader_program(shaders, shader_count);
}

static int init_shader_program(shader_program *prog, const char *name)
{
	prog->shader = compile_shader_program_v2(name);
	if (prog->shader == 0)
	{
		LOG_ERROR("Failed to compile shader: %s", name);
		return FAIL;
	}

	glUseProgram(prog->shader);
	prog->projection_loc = glGetUniformLocation(prog->shader, "uProjection");
	prog->resize_ratio_loc = glGetUniformLocation(prog->shader, "uResizeRatio");
	prog->block_width_loc = glGetUniformLocation(prog->shader, "uBlockWidth");
	prog->texture_loc = glGetUniformLocation(prog->shader, "uTexture");
	prog->color_loc = glGetUniformLocation(prog->shader, "uColor");
	prog->use_color_loc = glGetUniformLocation(prog->shader, "uUseColor");

	return SUCCESS;
}

static void cleanup_shader_program(shader_program *prog)
{
	if (prog->shader)
	{
		glDeleteProgram(prog->shader);
		prog->shader = 0;
	}
}

static int init_standard_renderer(shader_program *prog)
{
	glGenVertexArrays(1, &prog->vao);
	glBindVertexArray(prog->vao);

	glGenBuffers(1, &prog->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, prog->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &prog->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prog->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

	glGenBuffers(1, &prog->instance_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, prog->instance_vbo);

	size_t stride = sizeof(instance_data);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(instance_data, x));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(instance_data, scale_x));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(instance_data, rotation));
	glEnableVertexAttribArray(4);
	glVertexAttribDivisor(4, 1);

	glVertexAttribIPointer(5, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(instance_data, frame));
	glEnableVertexAttribArray(5);
	glVertexAttribDivisor(5, 1);

	glVertexAttribIPointer(6, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(instance_data, type));
	glEnableVertexAttribArray(6);
	glVertexAttribDivisor(6, 1);

	glVertexAttribIPointer(7, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(instance_data, flags));
	glEnableVertexAttribArray(7);
	glVertexAttribDivisor(7, 1);

	glBindVertexArray(0);

	return SUCCESS;
}

int renderer_v2_init(void)
{
	if (renderer_v2.initialized)
	{
		LOG_WARNING("Renderer already initialized");
		return SUCCESS;
	}

	memset(&renderer_v2, 0, sizeof(renderer_v2));

	if (init_shader_program(&renderer_v2.standard, "block") != SUCCESS)
		return FAIL;

	if (init_standard_renderer(&renderer_v2.standard) != SUCCESS)
		return FAIL;

	renderer_v2.batch.capacity = 10000;
	renderer_v2.batch.data = (instance_data *)malloc(renderer_v2.batch.capacity * sizeof(instance_data));
	if (!renderer_v2.batch.data)
	{
		LOG_ERROR("Failed to allocate instance buffer");
		return FAIL;
	}

	glBindBuffer(GL_ARRAY_BUFFER, renderer_v2.standard.instance_vbo);
	glBufferData(GL_ARRAY_BUFFER, renderer_v2.batch.capacity * sizeof(instance_data), NULL, GL_DYNAMIC_DRAW);

	renderer_v2.view_zoom = 1.0f;
	renderer_v2.view_w = SCREEN_WIDTH;
	renderer_v2.view_h = SCREEN_HEIGHT;

	glUseProgram(renderer_v2.standard.shader);
	glUniformMatrix4fv(renderer_v2.standard.projection_loc, 1, GL_FALSE,
					   (const float *)renderer_v2_projection_matrix(SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));
	glUniform1i(renderer_v2.standard.texture_loc, 0);

	glGenTextures(1, &renderer_v2.dummy_texture);
	glBindTexture(GL_TEXTURE_2D, renderer_v2.dummy_texture);
	{
		u8 px[4] = {255, 255, 255, 255};
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	renderer_v2.initialized = true;
	LOG_INFO("Renderer v2 initialized successfully");
	return SUCCESS;
}

void renderer_v2_shutdown(void)
{
	if (!renderer_v2.initialized)
		return;

	free(renderer_v2.batch.data);
	renderer_v2.batch.data = NULL;

	if (renderer_v2.standard.vao)
		glDeleteVertexArrays(1, &renderer_v2.standard.vao);
	if (renderer_v2.standard.vbo)
		glDeleteBuffers(1, &renderer_v2.standard.vbo);
	if (renderer_v2.standard.ebo)
		glDeleteBuffers(1, &renderer_v2.standard.ebo);
	if (renderer_v2.standard.instance_vbo)
		glDeleteBuffers(1, &renderer_v2.standard.instance_vbo);
	cleanup_shader_program(&renderer_v2.standard);

	if (renderer_v2.dummy_texture)
	{
		glDeleteTextures(1, &renderer_v2.dummy_texture);
		renderer_v2.dummy_texture = 0;
	}

	memset(&renderer_v2, 0, sizeof(renderer_v2));
	LOG_INFO("Renderer v2 shutdown complete");
}

void renderer_v2_begin_batch(GLuint texture, image *atlas_img, u8 block_width)
{
	renderer_v2.batch.count = 0;
	renderer_v2.batch.texture = texture;
	renderer_v2.batch.atlas_img = atlas_img;
	renderer_v2.batch.block_width = block_width;
}

int renderer_v2_add_instance(float x, float y, u8 frame, u8 type, u8 flags, float scale_x, float scale_y,
							 float rotation)
{
	if (renderer_v2.batch.count >= renderer_v2.batch.capacity)
	{
		u32 new_capacity = renderer_v2.batch.capacity * 2;
		instance_data *new_data =
			(instance_data *)realloc(renderer_v2.batch.data, new_capacity * sizeof(instance_data));
		if (!new_data)
		{
			LOG_ERROR("Failed to expand instance buffer");
			return FAIL;
		}

		renderer_v2.batch.capacity = new_capacity;
		renderer_v2.batch.data = new_data;

		glBindBuffer(GL_ARRAY_BUFFER, renderer_v2.standard.instance_vbo);
		glBufferData(GL_ARRAY_BUFFER, new_capacity * sizeof(instance_data), NULL, GL_DYNAMIC_DRAW);

		LOG_DEBUG("Expanded instance buffer to %u", new_capacity);
	}

	instance_data *inst = &renderer_v2.batch.data[renderer_v2.batch.count++];
	inst->x = x;
	inst->y = y;
	inst->scale_x = scale_x;
	inst->scale_y = scale_y;
	inst->rotation = rotation;
	inst->frame = frame;
	inst->type = type;
	inst->flags = flags;
	inst->padding = 0;

	return SUCCESS;
}

static void batch_ensure_gl(layer_batch *batch)
{
	if (batch->vao)
		return;

	shader_program *pg = &renderer_v2.standard;

	glGenVertexArrays(1, &batch->vao);
	glGenBuffers(1, &batch->instance_vbo);
	glBindVertexArray(batch->vao);

	glBindBuffer(GL_ARRAY_BUFFER, pg->vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pg->ebo);

	glBindBuffer(GL_ARRAY_BUFFER, batch->instance_vbo);
	size_t stride = sizeof(instance_data);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(instance_data, x));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(instance_data, scale_x));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(instance_data, rotation));
	glEnableVertexAttribArray(4);
	glVertexAttribDivisor(4, 1);

	glVertexAttribIPointer(5, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(instance_data, frame));
	glEnableVertexAttribArray(5);
	glVertexAttribDivisor(5, 1);

	glVertexAttribIPointer(6, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(instance_data, type));
	glEnableVertexAttribArray(6);
	glVertexAttribDivisor(6, 1);

	glVertexAttribIPointer(7, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(instance_data, flags));
	glEnableVertexAttribArray(7);
	glVertexAttribDivisor(7, 1);

	glBindVertexArray(0);
}

void renderer_v2_layer_begin(layer_batch *batch, GLuint texture, image *atlas_img, u8 block_width)
{
	batch->count = 0;
	batch->texture = texture;
	batch->atlas_img = atlas_img;
	batch->block_width = block_width;
}

void renderer_v2_layer_upload(layer_batch *batch, u32 offset, u32 count, const instance_data *src)
{
	if (!batch->vao)
		batch_ensure_gl(batch);

	u32 needed = offset + count;
	if (needed > batch->vbo_capacity)
	{
		u32 new_capacity = batch->vbo_capacity ? batch->vbo_capacity : 256;
		while (new_capacity < needed)
			new_capacity *= 2;

		glBindBuffer(GL_ARRAY_BUFFER, batch->instance_vbo);
		glBufferData(GL_ARRAY_BUFFER, new_capacity * sizeof(instance_data), NULL, GL_DYNAMIC_DRAW);
		batch->vbo_capacity = new_capacity;
	}

	glBindBuffer(GL_ARRAY_BUFFER, batch->instance_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(instance_data), count * sizeof(instance_data), src);
}

void renderer_v2_layer_draw(const layer_batch *batch, u32 count)
{
	if (!batch->vao || count == 0)
		return;

	shader_program *prog = &renderer_v2.standard;

	glUseProgram(prog->shader);
	glUniform1i(prog->use_color_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, batch->texture);
	glUniform2f(prog->resize_ratio_loc, (float)batch->atlas_img->width / g_block_width,
				(float)batch->atlas_img->height / g_block_width);
	glUniform1f(prog->block_width_loc, (float)batch->block_width);
	glBindVertexArray(batch->vao);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, count);
}

void renderer_v2_end_batch(void)
{
	layer_batch *batch = &renderer_v2.batch;
	if (batch->count == 0)
		return;

	renderer_v2_layer_upload(batch, 0, batch->count, batch->data);
	renderer_v2_layer_draw(batch, batch->count);
	batch->count = 0;
}

void renderer_v2_set_view(u16 width, u16 height, f32 cam_x, f32 cam_y, f32 zoom)
{
	renderer_v2.view_w = width;
	renderer_v2.view_h = height;
	renderer_v2.view_cam_x = cam_x;
	renderer_v2.view_cam_y = cam_y;
	renderer_v2.view_zoom = zoom > 0.0f ? zoom : 1.0f;

	if (!renderer_v2.initialized)
		return;

	float projection[16];
	renderer_v2_fill_projection(projection, width, height, cam_x, cam_y, renderer_v2.view_zoom);

	glUseProgram(renderer_v2.standard.shader);
	glUniformMatrix4fv(renderer_v2.standard.projection_loc, 1, GL_FALSE, projection);
}

void renderer_v2_resize(u16 width, u16 height)
{
	renderer_v2_set_view(width, height, renderer_v2.view_cam_x, renderer_v2.view_cam_y, renderer_v2.view_zoom);
}

void renderer_v2_set_projection_size(u16 width, u16 height)
{
	renderer_v2_set_view(width, height, renderer_v2.view_cam_x, renderer_v2.view_cam_y, renderer_v2.view_zoom);
}

void renderer_v2_fill_rect(f32 x, f32 y, f32 w, f32 h, const f32 color[4])
{
	if (!renderer_v2.initialized)
		return;

	f32 zoom = renderer_v2.view_zoom > 0.0f ? renderer_v2.view_zoom : 1.0f;

	shader_program *prog = &renderer_v2.standard;

	glUseProgram(prog->shader);
	glUniform1i(prog->use_color_loc, 1);
	glUniform4fv(prog->color_loc, 1, color);

	f32 wx = (x + renderer_v2.view_cam_x) / zoom;
	f32 wy = (y + renderer_v2.view_cam_y) / zoom;

	renderer_v2_begin_batch(renderer_v2.dummy_texture, &g_dummy_img, (u8)g_block_width);
	renderer_v2_add_instance(wx, wy, 0, 0, 0, w / zoom, h / zoom, 0.0f);
	renderer_v2_end_batch();

	glUniform1i(prog->use_color_loc, 0);
}
