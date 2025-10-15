#ifndef BLOCK_RENDERER_H
#define BLOCK_RENDERER_H

#include "general.h"
// #include "rendering.h"
#include "sdl2_basics.h"

typedef struct
{
    float x, y;
    float scale_x, scale_y;
    float rotation;
    u8 frame;
    u8 type;
    u8 flags;
    u8 padding;
} block_layer_instance;

typedef struct
{
    block_layer_instance *data;
    u32 instance_capacity;
    u32 instance_count;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    GLuint instance_vbo;

    GLuint shader;
} render_info;

typedef struct
{
    GLuint fbo;
    GLuint texture;
} framebuffer;

typedef struct
{
    render_info standard;

    GLuint atlas_resize_factor;
    GLuint projection_loc;
    GLuint block_width_loc;
    GLuint texture_loc;

    framebuffer post_framebuffer;

    render_info post;
} block_renderer;

extern block_renderer renderer;

framebuffer create_framebuffer_object(u16 width, u16 height);

int block_renderer_init();
void block_renderer_shutdown();

void block_renderer_update_size();

void block_renderer_begin_batch();
void block_renderer_end_batch(image *atlas_img, GLuint atlas, u8 local_block_width);

int block_renderer_create_instance(atlas_info info, int x, int y);
void block_renderer_set_instance_properties(u8 frame, u8 type, u8 flags, float scale_x, float scale_y, float rotation);

void block_renderer_begin_frame();
void block_renderer_end_frame();

// void block_renderer_render_frozen(layer_slice slice);

#endif // BLOCK_RENDERER_H