#include "../include/block_renderer.h"
#include "../include/opengl_stuff.h"

#include <stdlib.h>

// produced mostly by Claude 3.7 Sonnet

int block_renderer_init(block_renderer *renderer, texture *tex, int screenWidth,
                        int screenHeight)
{
    // Load shaders
    u32 shaders[2];
    shaders[0] =
        load_shader(FOLDER_SHD SEPARATOR_STR "block.vert", GL_VERTEX_SHADER);
    shaders[1] =
        load_shader(FOLDER_SHD SEPARATOR_STR "block.frag", GL_FRAGMENT_SHADER);

    if (!shaders[0] || !shaders[1])
    {
        LOG_ERROR("Failed to load block renderer shaders");
        return FAIL;
    }

    renderer->shaderProgram = compile_shader_program(shaders, 2);
    if (!renderer->shaderProgram)
    {
        LOG_ERROR("Failed to compile block renderer shader program");
        return FAIL;
    }

    // Get uniform locations
    renderer->projectionLoc =
        glGetUniformLocation(renderer->shaderProgram, "uProjection");
    renderer->frameCountLoc =
        glGetUniformLocation(renderer->shaderProgram, "uFrameCount");
    renderer->blockWidthLoc =
        glGetUniformLocation(renderer->shaderProgram, "uBlockWidth");
    renderer->textureLoc =
        glGetUniformLocation(renderer->shaderProgram, "uTexture");

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
    glGenVertexArrays(1, &renderer->vao);
    glBindVertexArray(renderer->vao);

    // Create and bind VBO
    glGenBuffers(1, &renderer->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create and bind EBO
    glGenBuffers(1, &renderer->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    // Create instance VBO (initially empty)
    glGenBuffers(1, &renderer->instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->instanceVBO);

    // Instance attribute (position, frame, type)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // Tell OpenGL this is an instanced attribute

    // Initialize instance data
    renderer->instanceCapacity = 1000; // Start with space for 1000 instances
    renderer->instanceData =
        (float *)malloc(renderer->instanceCapacity * 4 * sizeof(float));
    renderer->instanceCount = 0;

    // Set up orthographic projection
    float projection[16] = {0};
    // Simple orthographic projection matrix
    projection[0] = 2.0f / screenWidth;
    projection[5] = -2.0f / screenHeight; // Flip Y axis
    projection[10] = -1.0f;
    projection[12] = -1.0f;
    projection[13] = 1.0f;
    projection[15] = 1.0f;

    glUseProgram(renderer->shaderProgram);
    glUniformMatrix4fv(renderer->projectionLoc, 1, GL_FALSE, projection);
    glUniform1i(renderer->textureLoc, 0); // Use texture unit 0

    return SUCCESS;
}

void block_renderer_begin_batch(renderers_t renderers)
{
    for (u32 i = 0; i < renderers.length; i++)
        renderers.data[i].instanceCapacity = 0;
}

void block_renderer_add_block(block_renderer *renderer, int x, int y, u8 frame,
                              u8 type, u8 local_block_width)
{
    // Ensure we have enough capacity
    if (renderer->instanceCapacity == 0)
    {
        LOG_DEBUG("no instances???");

        renderer->instanceCapacity =
            1000; // Start with space for 1000 instances
        renderer->instanceData =
            (float *)malloc(renderer->instanceCapacity * 4 * sizeof(float));
        renderer->instanceCount = 0;
    }

    if (renderer->instanceCount >= renderer->instanceCapacity)
    {
        LOG_DEBUG("not enough instances, increasing to %d",
                  renderer->instanceCapacity * 2);
        renderer->instanceCapacity *= 2;
        renderer->instanceData =
            (float *)realloc(renderer->instanceData,
                             renderer->instanceCapacity * 4 * sizeof(float));
    }

    // LOG_DEBUG("add_block %d %d %d %d", x, y, frame, type);

    // Add instance data: x, y, frame, type
    int idx = renderer->instanceCount * 4;
    renderer->instanceData[idx + 0] = (float)x;
    renderer->instanceData[idx + 1] = (float)y;
    renderer->instanceData[idx + 2] = (float)frame;
    renderer->instanceData[idx + 3] = (float)type;

    renderer->instanceCount++;

    LOG_DEBUG("%d : putting %d %d %d %d in a shader, %d insances",
              renderer->shaderProgram, x, y, frame, type,
              renderer->instanceCount);
}

void block_renderer_end_batch(renderers_t renderers)
{
    for (u32 i = 0; i < renderers.length; i++)
    {
        block_renderer *renderer = &renderers.data[i];
        texture *tex = renderer->tex;

        if (renderer->instanceCount == 0)
            return;

        glUseProgram(renderer->shaderProgram);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->gl_id);

        // Update uniforms
        glUniform2f(renderer->frameCountLoc, (float)tex->frames,
                    (float)tex->types);
        glUniform1f(renderer->blockWidthLoc,
                    (float)renderer->local_block_width);

        // Update instance data
        glBindBuffer(GL_ARRAY_BUFFER, renderer->instanceVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     renderer->instanceCount * 4 * sizeof(float),
                     renderer->instanceData, GL_DYNAMIC_DRAW);

        // Draw instances
        glBindVertexArray(renderer->vao);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0,
                                renderer->instanceCount);
    }
}

// void block_renderer_shutdown(block_renderer *renderer)
// {
//     free(renderer->instanceData);

//     glDeleteVertexArrays(1, &renderer->vao);
//     glDeleteBuffers(1, &renderer->vbo);
//     glDeleteBuffers(1, &renderer->ebo);
//     glDeleteBuffers(1, &renderer->instanceVBO);
//     glDeleteProgram(renderer->shaderProgram);
// }

// Replacement for the original block_render function
int block_render_instanced(renderers_t renderers, const u64 id, const i32 x,
                           const i32 y, u8 frame, u8 type, u8 ignore_type,
                           u8 flip, u16 rotation)
{
    if (id == 0)
        return SUCCESS;

    block_renderer *renderer = NULL;

    for (u32 i = 0; i < renderers.length; i++)
        if (renderers.data[i].id == id)
        {
            renderer = &renderers.data[i];
            break;
        }

    texture *texture = renderer->tex;

    // frame is an index into one of the frames on a texture
    frame = frame % texture->total_frames; // wrap frames

    // Calculate the actual type based on ignore_type flag
    u8 actual_type =
        ignore_type ? (u8)(frame / texture->frames) : (type % texture->types);

    // Add this block to the batch
    block_renderer_add_block(renderer, x, y, frame % texture->frames,
                             actual_type, renderer->local_block_width);

    return SUCCESS;
}
