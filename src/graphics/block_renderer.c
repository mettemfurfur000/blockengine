#include "include/block_renderer.h"
#include "include/opengl_stuff.h"
#include "include/sdl2_basics.h"

#include <epoxy/gl_generated.h>
#include <stdlib.h>

block_renderer renderer = {0};

static render_info *std = &renderer.standard;

framebuffer create_framebuffer_object(u16 width, u16 height)
{
    framebuffer buffer = {};

    glGenFramebuffers(1, &buffer.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, buffer.fbo);

    glGenTextures(1, &buffer.texture);
    glBindTexture(GL_TEXTURE_2D, buffer.texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("Incomplete framebuffer");
        return (framebuffer){.fbo = 0, .texture = 0};
    }

    return buffer;
}

int block_renderer_init_post()
{
    if (renderer.post_framebuffer.fbo)
    {
        glDeleteFramebuffers(1, &renderer.post_framebuffer.fbo);
    }

    renderer.post_framebuffer = create_framebuffer_object(SCREEN_WIDTH, SCREEN_HEIGHT);

    if (renderer.post_framebuffer.fbo == 0)
    {
        LOG_ERROR("Failed to create a framebuffer for post processing");
        return FAIL;
    }

    renderer.post.shader = assemble_shader("post");
    if (!renderer.post.shader)
    {
        LOG_ERROR("Failed to compile post-processing shader program");
        return FAIL;
    }

    // Create fullscreen quad for post-processing
    float vertices[] = {
        // positions    // texture coords
        -1.0f, -1.0f, 0.0f, 0.0f, // bottom left
        1.0f,  -1.0f, 1.0f, 0.0f, // bottom right
        1.0f,  1.0f,  1.0f, 1.0f, // top right
        -1.0f, 1.0f,  0.0f, 1.0f  // top left
    };

    unsigned int indices[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    glUseProgram(renderer.post.shader);

    glGenVertexArrays(1, &renderer.post.vao);
    glBindVertexArray(renderer.post.vao);

    glGenBuffers(1, &renderer.post.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.post.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &renderer.post.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.post.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return SUCCESS;
}

int block_renderer_init()
{
    std->shader = assemble_shader("block");
    if (!std->shader)
    {
        LOG_ERROR("Failed to compile block renderer shader program");
        return FAIL;
    }

    glUseProgram(renderer.standard.shader);

    renderer.projection_loc = glGetUniformLocation(std->shader, "uProjection");
    renderer.atlas_resize_factor = glGetUniformLocation(std->shader, "uResizeRatio");
    renderer.block_width_loc = glGetUniformLocation(std->shader, "uBlockWidth");
    renderer.texture_loc = glGetUniformLocation(std->shader, "uTexture");

    float vertices[] = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f};

    unsigned int indices[] = {0, 1, 2, 2, 3, 0};

    glGenVertexArrays(1, &std->vao);
    glBindVertexArray(std->vao);

    glGenBuffers(1, &std->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, std->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &std->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glGenBuffers(1, &std->instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, std->instance_vbo);

    size_t stride = sizeof(block_layer_instance);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(block_layer_instance, x));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(block_layer_instance, scale_x));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(block_layer_instance, rotation));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glVertexAttribIPointer(5, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(block_layer_instance, frame));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glVertexAttribIPointer(6, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(block_layer_instance, type));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    glVertexAttribIPointer(7, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(block_layer_instance, flags));
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);

    std->instance_capacity = 1000;
    std->data = (block_layer_instance *)malloc(std->instance_capacity * sizeof(block_layer_instance));
    std->instance_count = 0;

    float projection[16] = {0};

    projection[0] = 2.0f / SCREEN_WIDTH;
    projection[5] = -2.0f / SCREEN_HEIGHT;
    projection[10] = -1.0f;
    projection[12] = -1.0f;
    projection[13] = 1.0f;
    projection[15] = 1.0f;

    glUseProgram(std->shader);

    glUniformMatrix4fv(renderer.projection_loc, 1, GL_FALSE, projection);
    glUniform1i(renderer.texture_loc, 0);

    if (block_renderer_init_post() != SUCCESS)
        return FAIL;
    return SUCCESS;
}

void block_renderer_begin_batch()
{
    std->instance_count = 0;
}

void block_renderer_end_batch(image *atlas_img, GLuint atlas, u8 local_block_width)
{
    if (std->instance_count == 0)
        return;

    glUseProgram(std->shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas);

    glUniform2f(renderer.atlas_resize_factor, (float)atlas_img->width / g_block_width,
                (float)atlas_img->height / g_block_width);
    glUniform1f(renderer.block_width_loc, (float)local_block_width);

    glBindBuffer(GL_ARRAY_BUFFER, std->instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, std->instance_count * sizeof(block_layer_instance), std->data, GL_DYNAMIC_DRAW);

    glBindVertexArray(std->vao);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, std->instance_count);
}

void block_renderer_shutdown()
{
    free(std->data);

    glDeleteVertexArrays(1, &std->vao);
    glDeleteBuffers(1, &std->vbo);
    glDeleteBuffers(1, &std->ebo);
    glDeleteBuffers(1, &std->instance_vbo);
    glDeleteProgram(std->shader);
}

int block_renderer_create_instance(atlas_info info, int x, int y)
{
    if (std->instance_count >= std->instance_capacity)
    {
        LOG_DEBUG("not enough instances, increasing to %d", std->instance_capacity * 2);
        std->instance_capacity *= 2;
        std->data = (block_layer_instance *)realloc(std->data, std->instance_capacity * sizeof(block_layer_instance));

        if (!std->data)
        {
            LOG_ERROR("Failed to allocate memory for instance data");
            return FAIL;
        }
    }

    block_layer_instance *instance = &std->data[std->instance_count];
    instance->x = (float)x;
    instance->y = (float)y;
    instance->scale_x = 1.0f;
    instance->scale_y = 1.0f;
    instance->rotation = 0.0f;
    instance->frame = info.atlas_offset_x;
    instance->type = info.atlas_offset_y;
    instance->flags = 0;

    std->instance_count++;
    return SUCCESS;
}

void block_renderer_set_instance_properties(u8 frame, u8 type, u8 flags, float scale_x, float scale_y, float rotation)
{
    if (std->instance_count == 0)
    {
        LOG_ERROR("No instance to configure properties for");
        return;
    }

    block_layer_instance *instance = &std->data[std->instance_count - 1];
    instance->frame = frame;
    instance->type = type;
    instance->flags = flags;
    instance->scale_x = scale_x;
    instance->scale_y = scale_y;
    instance->rotation = rotation;
}

void block_renderer_begin_frame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, renderer.post_framebuffer.fbo);
    glClear(GL_COLOR_BUFFER_BIT);
}

void block_renderer_end_frame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(renderer.post.shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer.post_framebuffer.texture);

    glBindVertexArray(renderer.post.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void block_renderer_update_size()
{
    float projection[16] = {0};

    projection[0] = 2.0f / SCREEN_WIDTH;
    projection[5] = -2.0f / SCREEN_HEIGHT;
    projection[10] = -1.0f;
    projection[12] = -1.0f;
    projection[13] = 1.0f;
    projection[15] = 1.0f;

    glUseProgram(std->shader);

    glUniformMatrix4fv(renderer.projection_loc, 1, GL_FALSE, projection);

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    block_renderer_init_post();
}