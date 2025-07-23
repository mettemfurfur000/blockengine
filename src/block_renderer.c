#include "../include/block_renderer.h"
#include "../include/opengl_stuff.h"

#include <epoxy/gl_generated.h>
#include <stdlib.h>

// produced mostly by Claude 3.7 Sonnet

typedef struct
{
    // Main rendering VAO/VBO
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    // Post-processing VAO/VBO
    GLuint postVao;
    GLuint postVbo;
    GLuint postEbo;

    GLuint fbo;
    GLuint fbo_texture;

    GLuint instanceVBO;
    GLuint shaderDefault;

    GLuint shaderPost;

    GLuint projectionLoc;
    GLuint atlasResizeFactor;
    GLuint blockWidthLoc;
    GLuint textureLoc;

    // Instance data buffer
    float *instanceData;
    int instanceCapacity;
    int instanceCount;
} BlockRenderer;

static BlockRenderer renderer = {0};

void create_framebuffer_object(u16 screenWidth, u16 screenHeight, u32 *fbo_ref, u32 *tex_ref)
{
    CHECK_PTR_NORET(fbo_ref)
    CHECK_PTR_NORET(tex_ref)

    glGenFramebuffers(1, fbo_ref);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo_ref);

    glGenTextures(1, tex_ref);
    glBindTexture(GL_TEXTURE_2D, *tex_ref);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex_ref, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("Incomplete framebuffer");
        return;
    }
}

int block_renderer_init(int screenWidth, int screenHeight)
{
    // Load shaders
    renderer.shaderDefault = assemble_shader("block");
    if (!renderer.shaderDefault)
    {
        LOG_ERROR("Failed to compile block renderer shader program");
        return FAIL;
    }

    renderer.shaderPost = assemble_shader("post");
    if (!renderer.shaderPost)
    {
        LOG_ERROR("Failed to compile post-processing shader program");
        return FAIL;
    }

    // Get uniform locations
    renderer.projectionLoc = glGetUniformLocation(renderer.shaderDefault, "uProjection");
    renderer.atlasResizeFactor = glGetUniformLocation(renderer.shaderDefault, "uResizeRatio");
    renderer.blockWidthLoc = glGetUniformLocation(renderer.shaderDefault, "uBlockWidth");
    renderer.textureLoc = glGetUniformLocation(renderer.shaderDefault, "uTexture");

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
    glGenVertexArrays(1, &renderer.vao);
    glBindVertexArray(renderer.vao);

    // Create and bind VBO
    glGenBuffers(1, &renderer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create and bind EBO
    glGenBuffers(1, &renderer.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Create instance VBO (initially empty)
    glGenBuffers(1, &renderer.instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.instanceVBO);

    // Instance attribute
    glVertexAttribPointer(2, INSTANCE_FIELDS_COUNT, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // Tell OpenGL this is an instanced attribute

    // Initialize instance data
    renderer.instanceCapacity = 1000; // Start with space for 1000 instances
    renderer.instanceData = (float *)malloc(renderer.instanceCapacity * INSTANCE_FIELDS_COUNT * sizeof(float));
    renderer.instanceCount = 0;

    // Set up orthographic projection
    float projection[16] = {0};
    // Simple orthographic projection matrix
    projection[0] = 2.0f / screenWidth;
    projection[5] = -2.0f / screenHeight; // Flip Y axis
    projection[10] = -1.0f;
    projection[12] = -1.0f;
    projection[13] = 1.0f;
    projection[15] = 1.0f;

    // Use the shader right away

    glUseProgram(renderer.shaderDefault);
    glUniformMatrix4fv(renderer.projectionLoc, 1, GL_FALSE, projection);
    glUniform1i(renderer.textureLoc, 0);

    create_framebuffer_object(screenWidth, screenHeight, &renderer.fbo, &renderer.fbo_texture);

    // Create fullscreen quad for post-processing
    float postVertices[] = {
        // positions    // texture coords
        -1.0f, -1.0f, 0.0f, 0.0f, // bottom left
        1.0f,  -1.0f, 1.0f, 0.0f, // bottom right
        1.0f,  1.0f,  1.0f, 1.0f, // top right
        -1.0f, 1.0f,  0.0f, 1.0f  // top left
    };

    unsigned int postIndices[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    // Create and set up post-processing VAO/VBO
    glGenVertexArrays(1, &renderer.postVao);
    glBindVertexArray(renderer.postVao);

    glGenBuffers(1, &renderer.postVbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.postVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(postVertices), postVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &renderer.postEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.postEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(postIndices), postIndices, GL_STATIC_DRAW);

    // Set up attributes for post-processing quad
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return SUCCESS;
}

void block_renderer_begin_batch()
{
    renderer.instanceCount = 0;
}

void block_renderer_end_batch(image *atlas_img, GLuint atlas, u8 local_block_width)
{
    if (renderer.instanceCount == 0)
        return;

    glUseProgram(renderer.shaderDefault);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas);

    // Update uniforms
    glUniform2f(renderer.atlasResizeFactor, (float)atlas_img->width / g_block_width,
                (float)atlas_img->height / g_block_width);
    glUniform1f(renderer.blockWidthLoc, (float)local_block_width);

    // Update instance data
    glBindBuffer(GL_ARRAY_BUFFER, renderer.instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, renderer.instanceCount * INSTANCE_FIELDS_COUNT * sizeof(float), renderer.instanceData,
                 GL_DYNAMIC_DRAW);

    // Draw instances
    glBindVertexArray(renderer.vao);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, renderer.instanceCount);
}

void block_renderer_shutdown()
{
    free(renderer.instanceData);

    glDeleteVertexArrays(1, &renderer.vao);
    glDeleteBuffers(1, &renderer.vbo);
    glDeleteBuffers(1, &renderer.ebo);
    glDeleteBuffers(1, &renderer.instanceVBO);
    glDeleteProgram(renderer.shaderDefault);
}

// Replacement for the original block_render function
int block_render_instanced(atlas_info info, const int x, const int y, u8 frame, u8 type, u8 ignore_type, u8 flip)
{
    u8 actual_frame = frame % info.frames;
    u8 actual_type = ignore_type ? (u8)(frame / info.frames) : (type % info.types);

    // Ensure we have enough capacity
    if (renderer.instanceCount >= renderer.instanceCapacity)
    {
        LOG_DEBUG("not enough instances, increasing to %d", renderer.instanceCount * INSTANCE_FIELDS_COUNT);
        renderer.instanceCapacity *= 2;
        renderer.instanceData =
            (float *)realloc(renderer.instanceData, renderer.instanceCapacity * INSTANCE_FIELDS_COUNT * sizeof(float));
    }

    // Add instance data: x, y, frame, type
    int idx = renderer.instanceCount * INSTANCE_FIELDS_COUNT;
    renderer.instanceData[idx + 0] = (float)x;
    renderer.instanceData[idx + 1] = (float)y;
    renderer.instanceData[idx + 2] = (float)(info.atlas_offset_x + actual_frame);
    renderer.instanceData[idx + 3] = (float)(info.atlas_offset_y + actual_type);

    // LOG_DEBUG("pos: %dx%d, offset: %dx%d, t&f: %dx%d", x, y,
    // info.atlas_offset_x, info.atlas_offset_y, frame, actual_type);

    renderer.instanceCount++;

    return SUCCESS;
}

void block_renderer_begin_frame()
{
    // Bind our framebuffer to render into it
    glBindFramebuffer(GL_FRAMEBUFFER, renderer.fbo);
    glClear(GL_COLOR_BUFFER_BIT);
}

void block_renderer_end_frame()
{
    // Switch back to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Clear the default framebuffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Use post-processing shader
    glUseProgram(renderer.shaderPost);

    // Bind the framebuffer texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer.fbo_texture);

    // Draw the fullscreen quad using the post-processing VAO
    glBindVertexArray(renderer.postVao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
