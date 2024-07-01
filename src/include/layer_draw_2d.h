#ifndef LAYER_DRAW_2D
#define LAYER_DRAW_2D

#include "sdl2_basics.h"
#include "block_registry.h"
#include "block_operations.h"

typedef struct client_view_point
{
	int screen_width, screen_height;

	int x, y;
	float scale;

	vec_int_t draw_order;
} client_view_point;

// renders layer of the world. smart enough to not render what player doesnt see
void layer_render(const world *w, const int layer_index, block_registry_t *b_reg,
				  const int frame_number,
				  const int width, const int height,
				  const client_view_point view);

// renders single frame of the world.
void client_render(const world *w, block_registry_t *b_reg, client_view_point view_point, const int frame_number);

// TODO: after handling block updates from server, make sure to write function that rerenders only changed parts of the world.
// until then, just rerender everything.

#endif