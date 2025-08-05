#ifndef RENDERING_H
#define RENDERING_H

// #include "sdl2_basics.h"
// #include "block_registry.h"
// #include "vars.h"
#include "level.h"

#define LAYER_SLICE_FLAG_FROZEN 0b00000001
#define LAYER_SLICE_FLAG_RENDER_COMPLETE 0b00000010

typedef struct layer_slice
{
    layer *ref;
    u32 framebuffer;         // OpenGL framebuffer object ID
    u32 framebuffer_texture; // OpenGL texture ID for the framebuffer

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

// moved to level.h since it has no connection to rendering
// void bprintf(layer *l, int orig_x, int orig_y, int length_limit, char
// *format, ...);

// u8 render_layer(layer_slice slice);
u8 client_render(const client_render_rules rules);

#endif