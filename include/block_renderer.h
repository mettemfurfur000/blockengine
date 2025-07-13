#ifndef BLOCK_RENDERER_H
#define BLOCK_RENDERER_H

#include "general.h"
#include "sdl2_basics.h"

#define INSTANCE_FIELDS_COUNT 4 // cannot be more than 4

// Initialize the block renderer
int block_renderer_init(int screenWidth, int screenHeight);

// Begin a new batch of blocks
void block_renderer_begin_batch();

// Add a block to the current batch
void block_renderer_add_block(int x, int y, u8 frame, u8 type, u8 a_offset_x,
                              u8 a_offset_y);

void block_renderer_end_batch(image *atlas_img, GLuint atlas,
                              u8 local_block_width);

// Clean up resources
void block_renderer_shutdown();

// Replacement for the original block_render function
int block_render_instanced(atlas_info info, const int x, const int y, u8 frame,
                           u8 type, u8 ignore_type, u8 flip);

#endif // BLOCK_RENDERER_H