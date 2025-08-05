#ifndef BLOCK_RENDERER_H
#define BLOCK_RENDERER_H

#include "general.h"
#include "sdl2_basics.h"
#include "rendering.h"

// Instance data structure
typedef struct
{
    float x, y;             // Position (8 bytes)
    float scale_x, scale_y; // Scale factors for stretching (8 bytes)
    float rotation;         // Rotation in radians (4 bytes)
    u8 frame;               // Current animation frame (1 byte)
    u8 type;                // Block type (1 byte)
    u8 flags;               // Bit flags for flip, special effects etc (1 byte)
    u8 padding;             // Padding for alignment (1 byte)
} block_layer_instance;     // Total: 24 bytes

// All the info needed to render stuff
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
    // Main rendering VAO/VBO
    render_info standard;

    // uniforms for standard shader
    GLuint atlas_resize_factor;
    GLuint projection_loc;
    GLuint block_width_loc;
    GLuint texture_loc;

    framebuffer post_framebuffer;

    // Post-processing VAO/VBO
    render_info post;

    // Static layer renderer
    render_info frozen_renderer;
} block_renderer;

extern block_renderer renderer;

// create framebuffers if needed
// void create_framebuffer_object(u16 screenWidth, u16 screenHeight, u32 *fbo_ref, u32 *tex_ref);
framebuffer create_framebuffer_object(u16 width, u16 height);

// Initialize the block renderer
int block_renderer_init(int screenWidth, int screenHeight);

// Begin a new batch of blocks
void block_renderer_begin_batch();

void block_renderer_end_batch(image *atlas_img, GLuint atlas, u8 local_block_width);

// Clean up resources
void block_renderer_shutdown();

// Create a new block instance at the specified position
int block_renderer_create_instance(atlas_info info, int x, int y);

// Configure properties for the last created instance
void block_renderer_set_instance_properties(u8 frame, u8 type, u8 flags, float scale_x, float scale_y, float rotation);

void block_renderer_begin_frame();
void block_renderer_end_frame();

void block_renderer_render_frozen(layer_slice slice);

#endif // BLOCK_RENDERER_H