#ifndef RENDERING_H
#define RENDERING_H

#include "level.h"

#define LAYER_SLICE_FLAG_FROZEN 0b00000001
#define LAYER_SLICE_FLAG_RENDER_COMPLETE 0b00000010

typedef struct layer_slice
{
    layer *ref;
    u32 framebuffer;
    u32 framebuffer_texture;

    u32 x, y; // coordinates in the world, in pixels (most of the time 16 per block)
    u32 w, h; // width and height of visible area

    u8 zoom;
    u8 flags; // static layer gets rendered once in a framebuffer
} layer_slice;

typedef vec_t(layer_slice) layer_slices_t;

typedef struct client_render_rules
{
    u16 screen_width, screen_height;
    u64 cur_frame;

    layer_slices_t slices;
    vec_int_t draw_order;
} client_render_rules;

u8 client_render(const client_render_rules rules);

#endif