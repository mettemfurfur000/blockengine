#ifndef BLOCK_RENDERER_H
#define BLOCK_RENDERER_H 1

// #include "block_registry.h"
#include "general.h"
#include "sdl2_basics.h"

typedef struct block_renderer
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint instanceVBO;
    GLuint shaderProgram;
    GLuint projectionLoc;
    GLuint frameCountLoc;
    GLuint blockWidthLoc;
    GLuint textureLoc;

    // Instance data buffer
    float *instanceData;
    int instanceCapacity;
    int instanceCount;

    // A texture associated with this renderer
    texture *tex;
    u64 id; // block registry block id
    u8 local_block_width;
} block_renderer;

typedef vec_t(block_renderer) renderers_t;

int block_renderer_init(block_renderer *renderer, texture *tex, int screenWidth,
                        int screenHeight);

void block_renderer_begin_batch(renderers_t renderers);
void block_renderer_end_batch(renderers_t renderers);

// Replacement for the original block_render function
int block_render_instanced(renderers_t renderers, const u64 id, const i32 x,
                           const i32 y, u8 frame, u8 type, u8 ignore_type,
                           u8 flip, u16 rotation);

// renderers_t block_renderers_generate(block_registry *reg);
// void block_renderers_close(renderers_t renderers);

#endif // BLOCK_RENDERER_H