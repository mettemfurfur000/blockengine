#include "include/layer_draw_2d.h"

// renders layer of the world. smart enough to not render what player doesnt see
void layer_render(const world *w, const int layer_index, block_registry_t *b_reg,
				  const int frame_number,
				  const int width, const int height,
				  const client_view_point view)
{
	if (!w || !b_reg)
		return;
	if (layer_index < 0 || layer_index >= w->depth)
		return;
	if (width < 0 || height < 0)
		return;

	texture *texture;
	block *b;
	SDL_Rect src_rect;
	SDL_Rect dest_rect = {0, 0, g_block_size, g_block_size};

	const int scaled_view_width = view.screen_width / view.scale;
	const int scaled_view_height = view.screen_height / view.scale;

	const int start_block_x = (int)(((view.x - scaled_view_width / 2) / g_block_size) - 1);
	const int start_block_y = (int)(((view.y - scaled_view_height / 2) / g_block_size) - 1);

	const int end_block_x = start_block_x + width + 1;
	const int end_block_y = start_block_y + height + 1;

	// loop over blocks

	// calculate x coordinate of block on screen
	const int block_x_offset = (view.x - scaled_view_width / 2) % g_block_size; /* offset in pixels for smooth rendering of blocks */
	const int block_y_offset = (view.y - scaled_view_height / 2) % g_block_size;

	printf("block_offsets: %d %d\n", block_x_offset, block_y_offset);

	// dest_rect.x = start_block_x * scaled_block_size + (scaled_view_width / 2 - view.x) - block_x_offset;

	float dest_x, dest_y;

	dest_x = -block_x_offset;

	for (int i = start_block_x; i < end_block_x; i++)
	{
		dest_x += g_block_size;
		// dest_rect.y = start_block_y * scaled_block_size + (scaled_view_height / 2 - view.y) - block_y_offset;
		dest_y = -block_y_offset;

		for (int j = start_block_y; j < end_block_y; j++)
		{
			dest_y += g_block_size;
			// calculate y coordinate of block on screen

			// get block
			if (!(b = get_block_access(w, layer_index, i, j)))
				continue;
			// check if block is not void
			if (b->id == 0)
				continue;
			// check if block is not from the same registry
			if (b->id != b_reg->data[b->id].block_sample.id)
				continue;
			// check if block could exist in registry
			if (b->id >= b_reg->length)
				continue;
			// get texture
			texture = &b_reg->data[b->id].block_texture;
			if (!texture)
				continue;
			// get frame
			if (texture->frames > 1)
			{
				int frame = frame_number % texture->frames;
				src_rect.x = texture->frame_side_size * (frame % texture->frames_per_line);
				src_rect.y = texture->frame_side_size * (frame / texture->frames_per_line);
			}
			else
			{
				src_rect.x = 0;
				src_rect.y = 0;
			}
			// source is for from what part of texture render
			src_rect.h = texture->frame_side_size;
			src_rect.w = texture->frame_side_size;
			// and dest_rect is for where on window render it
			dest_rect.x = dest_x;
			dest_rect.y = dest_y;
			// no camera rotating, yet...
			SDL_RenderCopy(g_renderer, texture->ptr, &src_rect, &dest_rect);
		}
	}
}

// renders single frame of the world.
void client_render(const world *w, block_registry_t *b_reg, client_view_point view_point, const int frame_number)
{
	// calculate how much blocks to render
	const int blocks_to_render_width = view_point.screen_width / g_block_size + 2;
	const int block_to_render_height = view_point.screen_height / g_block_size + 2;

	for (int i = 0; i < view_point.draw_order.length; i++)
		layer_render(w, view_point.draw_order.data[i], b_reg, frame_number,
					 blocks_to_render_width,
					 block_to_render_height,
					 view_point);
}

// TODO: after handling block updates from server, make sure to write function that rerenders only changed parts of the world.
// until then, just rerender everything.
