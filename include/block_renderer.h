#ifndef BLOCK_RENDERER_H
#define BLOCK_RENDERER_H

#include "general.h"
#include "sdl2_basics.h"

// Instance data structure
typedef struct {
    float x, y;           // Position (8 bytes)
    float scaleX, scaleY; // Scale factors for stretching (8 bytes)
    float rotation;       // Rotation in radians (4 bytes)
    u8 frame;            // Current animation frame (1 byte)
    u8 type;             // Block type (1 byte)
    u8 flags;            // Bit flags for flip, special effects etc (1 byte)
    u8 padding;          // Padding for alignment (1 byte)
} BlockInstanceData;      // Total: 24 bytes

// create framebuffers if needed
void create_framebuffer_object(u16 screenWidth, u16 screenHeight, u32 *fbo_ref, u32 *tex_ref);
void block_renderer_bind_custom_framebuffer(int fb);
void block_renderer_render_fb_texture_to_main_fb(int texutre);

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
void block_renderer_set_instance_properties(u8 frame, u8 type, u8 flags, float scaleX, float scaleY, float rotation);

void block_renderer_begin_frame();
void block_renderer_end_frame();

#endif // BLOCK_RENDERER_H