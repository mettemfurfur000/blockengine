#ifndef RENDERING_H
#define RENDERING_H

#include "sdl2_basics.h"
#include "block_registry.h"
#include "vars.h"
#include "level.h"

typedef struct layer_slice
{
    layer *ref;

    u32 x, y; // coordinates in the world, in pixels ( most of the time 16 per block )
    u32 w, h; // width an height of visible uhhhh

    u8 zoom;
} layer_slice;

typedef vec_t(layer_slice) layer_slices_t;

typedef struct client_render_rules
{
    u16 screen_width, screen_height;
    u64 cur_frame;

    layer_slices_t slices;
    vec_int_t draw_order;
} client_render_rules;

u8 render_layer(layer_slice slice);
u8 client_render(const client_render_rules rules);

#endif