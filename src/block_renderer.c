#include "../include/block_renderer.h"
#include "../include/opengl_stuff.h"

#include <epoxy/gl_generated.h>
#include <stdlib.h>

// produced mostly by Claude 3.7 Sonnet

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

int block_renderer_init_static(int width, int height)
{
    renderer.frozen_renderer.shader = assemble_shader("static");
    if (!renderer.frozen_renderer.shader)
    {
        LOG_ERROR("Failed to compile static layer shader program");
        return FAIL;
    }

    return SUCCESS;
}

int block_renderer_init_post(int width, int height)
{
    // prepare post-processing
    renderer.post_framebuffer = create_framebuffer_object(width, height);
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

    // Create and set up post-processing VAO/VBO
    glGenVertexArrays(1, &renderer.post.vao);
    glBindVertexArray(renderer.post.vao);

    glGenBuffers(1, &renderer.post.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.post.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &renderer.post.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.post.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Set up attributes for post-processing quad
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return SUCCESS;
}

int block_renderer_init(int width, int height)
{
    // Load shaders
    std->shader = assemble_shader("block");
    if (!std->shader)
    {
        LOG_ERROR("Failed to compile block renderer shader program");
        return FAIL;
    }

    // Get uniform locations
    renderer.projection_loc = glGetUniformLocation(std->shader, "uProjection");
    renderer.atlas_resize_factor = glGetUniformLocation(std->shader, "uResizeRatio");
    renderer.block_width_loc = glGetUniformLocation(std->shader, "uBlockWidth");
    renderer.texture_loc = glGetUniformLocation(std->shader, "uTexture");

    // Create vertex data for a quad
    float vertices[] = {
        // positions    // texture coords
        0.0f, 0.0f, 0.0f, 0.0f, // bottom left
        1.0f, 0.0f, 1.0f, 0.0f, // bottom right
        1.0f, 1.0f, 1.0f, 1.0f, // top right
        0.0f, 1.0f, 0.0f, 1.0f  // top left
    };

    unsigned int indices[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    // Create and bind VAO
    glGenVertexArrays(1, &std->vao);
    glBindVertexArray(std->vao);

    // Create and bind VBO
    glGenBuffers(1, &std->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, std->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create and bind EBO
    glGenBuffers(1, &std->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Create instance VBO (initially empty)
    glGenBuffers(1, &std->instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, std->instance_vbo);

    // Set up instance attributes
    size_t stride = sizeof(block_layer_instance);

    // Position (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(block_layer_instance, x));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Scale (vec2)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(block_layer_instance, scale_x));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // Rotation (float)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(block_layer_instance, rotation));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // Frame (uint8)
    glVertexAttribIPointer(5, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(block_layer_instance, frame));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    // Type (uint8)
    glVertexAttribIPointer(6, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(block_layer_instance, type));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    // Flags (uint8)
    glVertexAttribIPointer(7, 1, GL_UNSIGNED_BYTE, stride, (void *)offsetof(block_layer_instance, flags));
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);

    // Initialize instance data
    std->instance_capacity = 1000; // Start with space for 1000 instances
    std->data = (block_layer_instance *)malloc(std->instance_capacity * sizeof(block_layer_instance));
    std->instance_count = 0;

    // Set up orthographic projection
    float projection[16] = {0};
    // Simple orthographic projection matrix
    projection[0] = 2.0f / width;
    projection[5] = -2.0f / height; // Flip Y axis
    projection[10] = -1.0f;
    projection[12] = -1.0f;
    projection[13] = 1.0f;
    projection[15] = 1.0f;

    // Use the shader right away
    glUseProgram(std->shader);
    glUniformMatrix4fv(renderer.projection_loc, 1, GL_FALSE, projection);
    glUniform1i(renderer.texture_loc, 0);

    // Initialize post-processing and static layer rendering
    if (block_renderer_init_post(width, height) != SUCCESS)
        return FAIL;
    if (block_renderer_init_static(width, height) != SUCCESS)
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

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas);

    // Update uniforms
    glUniform2f(renderer.atlas_resize_factor, (float)atlas_img->width / g_block_width,
                (float)atlas_img->height / g_block_width);
    glUniform1f(renderer.block_width_loc, (float)local_block_width);

    // Update instance data
    glBindBuffer(GL_ARRAY_BUFFER, std->instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, std->instance_count * sizeof(block_layer_instance), std->data, GL_DYNAMIC_DRAW);

    // Draw instances
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

// Create a new block instance at the specified position
int block_renderer_create_instance(atlas_info info, int x, int y)
{
    // Ensure we have enough capacity
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

    // Add instance data with default properties
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

// Configure properties for the last created instance
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
    // Bind our framebuffer to render into it
    glBindFramebuffer(GL_FRAMEBUFFER, renderer.post_framebuffer.fbo);
    glClear(GL_COLOR_BUFFER_BIT);
}

void block_renderer_bind_custom_framebuffer(framebuffer fb)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
    glClear(GL_COLOR_BUFFER_BIT);
}

void block_renderer_end_frame()
{
    // Switch back to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Clear the default framebuffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Use post-processing shader
    glUseProgram(renderer.post.shader);

    // Bind the framebuffer texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer.post_framebuffer.texture);

    // Draw the fullscreen quad using the post-processing VAO
    glBindVertexArray(renderer.post.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void block_renderer_render_frozen(layer_slice *slice)
{
    // Switch to static shader for final composition
    GLuint frozen_shader = renderer.frozen_renderer.shader;
    glUseProgram(frozen_shader);

    // Set the offset and scale uniforms for proper positioning
    float offset[2] = {(float)slice->x, (float)slice->y};
    float scale[2] = {(float)slice->w, (float)slice->h};

    // also take into account slice scaling
    scale[0] /= slice->zoom;
    scale[1] /= slice->zoom;

    // Set uniforms for the static layer shader
    glUniform2fv(glGetUniformLocation(frozen_shader, "uOffset"), 1, offset);
    glUniform2fv(glGetUniformLocation(frozen_shader, "uSize"), 1, scale);

    // // Set texture uniform and bind the texture
    // glUniform1i(glGetUniformLocation(frozen_shader, "uTexture"), 0); // Use texture unit 0
    // glActiveTexture(GL_TEXTURE0);
    // // The texture should already be bound from the previous rendering pass
    glBindTexture(GL_TEXTURE_2D, slice->framebuffer_texture);

    // Use the post-processing VAO since it's already set up for fullscreen quads
    glBindVertexArray(renderer.post.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}