#ifndef LAYER_DRAW_2D
#define LAYER_DRAW_2D

#include "sdl2_basics.h"
#include "block_registry.h"
#include "block_operations.h"
#include "data_manipulations.h"
#include "world_utils.h"

typedef struct layer_slice
{
	int x, y; // coordinates in the world, in pixels ( most of the time 16 per block )
	int w, h; // width an height of visible uhhhh

	float scale;
} layer_slice;

typedef vec_t(layer_slice) layer_slices_t;

typedef struct client_render_rules
{
	int screen_width, screen_height;

	layer_slices_t slices;
	vec_int_t draw_order;
} client_render_rules;

// renders layer of the world. smart enough to not render what player doesnt see
void layer_render(const world *w, const int layer_index, block_registry_t *b_reg,
				  const int frame_number,
				  const layer_slice slice);

// renders single frame of the world.
void client_render(const world *w, block_registry_t *b_reg, client_render_rules render_rules, const int cur_frame);

// TODO: after handling block updates from server, make sure to write function that rerenders only changed parts of the world.
// until then, just rerender everything.

#endif