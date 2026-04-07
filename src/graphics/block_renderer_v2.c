#include "include/block_renderer_v2.h"

#include "include/folder_structure.h"
#include "include/logging.h"
#include "include/opengl_stuff.h"
#include "include/sdl2_basics.h"

#include <epoxy/gl_generated.h>

#include <stdlib.h>
#include <string.h>

block_renderer_v2 renderer_v2 = {0};

static const float quad_vertices[] = {
	0.0f, 0.0f, 0.0f, 0.0f, //
	1.0f, 0.0f, 1.0f, 0.0f, //
	1.0f, 1.0f, 1.0f, 1.0f, //
	0.0f, 1.0f, 0.0f, 1.0f	//
};

static const unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

static const float post_vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
									  1.0f,	 1.0f,	1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

static const unsigned int post_indices[] = {0, 1, 2, 2, 3, 0};

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

static int init_post_processing(shader_program *prog)
{
	glGenVertexArrays(1, &prog->vao);
	glBindVertexArray(prog->vao);

	glGenBuffers(1, &prog->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, prog->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(post_vertices), post_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &prog->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prog->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(post_indices), post_indices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	return SUCCESS;
}

static void cleanup_framebuffer(GLuint *fbo, GLuint *texture)
{
	if (*fbo)
	{
		glDeleteFramebuffers(1, fbo);
		*fbo = 0;
	}
	if (*texture)
	{
		glDeleteTextures(1, texture);
		*texture = 0;
	}
}

static int create_framebuffer(GLuint *fbo, GLuint *texture, u16 width, u16 height)
{
	cleanup_framebuffer(fbo, texture);

	glGenFramebuffers(1, fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG_ERROR("Incomplete framebuffer");
		cleanup_framebuffer(fbo, texture);
		return FAIL;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	float projection[16] = {0};
	projection[0] = 2.0f / SCREEN_WIDTH;
	projection[5] = -2.0f / SCREEN_HEIGHT;
	projection[10] = -1.0f;
	projection[12] = -1.0f;
	projection[13] = 1.0f;
	projection[15] = 1.0f;

	glUseProgram(renderer_v2.standard.shader);
	glUniformMatrix4fv(renderer_v2.standard.projection_loc, 1, GL_FALSE, projection);
	glUniform1i(renderer_v2.standard.texture_loc, 0);

	if (init_shader_program(&renderer_v2.post, "post") != SUCCESS)
		return FAIL;

	if (init_post_processing(&renderer_v2.post) != SUCCESS)
		return FAIL;

	if (create_framebuffer(&renderer_v2.post_fbo, &renderer_v2.post_texture, SCREEN_WIDTH, SCREEN_HEIGHT) != SUCCESS)
		return FAIL;

	glUseProgram(renderer_v2.post.shader);
	glUniform1i(glGetUniformLocation(renderer_v2.post.shader, "uTexture"), 0);

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

	if (renderer_v2.post.vao)
		glDeleteVertexArrays(1, &renderer_v2.post.vao);
	if (renderer_v2.post.vbo)
		glDeleteBuffers(1, &renderer_v2.post.vbo);
	if (renderer_v2.post.ebo)
		glDeleteBuffers(1, &renderer_v2.post.ebo);
	cleanup_shader_program(&renderer_v2.post);

	cleanup_framebuffer(&renderer_v2.post_fbo, &renderer_v2.post_texture);

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

void renderer_v2_end_batch(void)
{
	if (renderer_v2.batch.count == 0)
		return;

	shader_program *prog = &renderer_v2.standard;
	layer_batch *batch = &renderer_v2.batch;

	glUseProgram(prog->shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, batch->texture);

	glUniform2f(prog->resize_ratio_loc, (float)batch->atlas_img->width / g_block_width,
				(float)batch->atlas_img->height / g_block_width);
	glUniform1f(prog->block_width_loc, (float)batch->block_width);

	glBindBuffer(GL_ARRAY_BUFFER, prog->instance_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * sizeof(instance_data), batch->data);

	glBindVertexArray(prog->vao);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, batch->count);
}

void renderer_v2_begin_frame(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, renderer_v2.post_fbo);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT);
}

void renderer_v2_end_frame(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(renderer_v2.post.shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer_v2.post_texture);

	glBindVertexArray(renderer_v2.post.vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void renderer_v2_resize(u16 width, u16 height)
{
	if (!renderer_v2.initialized)
		return;

	create_framebuffer(&renderer_v2.post_fbo, &renderer_v2.post_texture, width, height);

	float projection[16] = {0};
	projection[0] = 2.0f / width;
	projection[5] = -2.0f / height;
	projection[10] = -1.0f;
	projection[12] = -1.0f;
	projection[13] = 1.0f;
	projection[15] = 1.0f;

	glUseProgram(renderer_v2.standard.shader);
	glUniformMatrix4fv(renderer_v2.standard.projection_loc, 1, GL_FALSE, projection);
}
