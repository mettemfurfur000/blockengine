#ifndef BLOCK_RENDERER_H
#define BLOCK_RENDERER_H

#include "general.h"
#include "sdl2_basics.h"

// Initialize the block renderer
int block_renderer_init(int screenWidth, int screenHeight);

// Begin a new batch of blocks
void block_renderer_begin_batch();

// Add a block to the current batch
void block_renderer_add_block(int x, int y, u8 frame, u8 type, u8 local_block_width);

// Render all blocks in the current batch with the given texture
void block_renderer_end_batch(texture *tex, u8 local_block_width);

// Clean up resources
void block_renderer_shutdown();

// Replacement for the original block_render function
int block_render_instanced(texture *texture, const int x, const int y, u8 frame, u8 type,
                 u8 ignore_type, u8 local_block_width, u8 flip,
                 unsigned short rotation);

#endif // BLOCK_RENDERER_H