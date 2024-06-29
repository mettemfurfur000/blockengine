#include "include/layer_draw_2d.h"

extern SDL_Window *g_window;

extern SDL_Renderer *g_renderer;

const int block_size = 16;

// renders layer of the world. smart enough to not render what player doesnt see
void layer_render(const world *w, const int layer_index, block_resources *b_res,
				  const int frame_number,
				  const int width, const int height,
				  const float scaled_block_size,
				  const client_view_point view)
{
	if (!w || !b_res)
		return;
	if (layer_index < 0 || layer_index >= w->depth)
		return;
	if (width < 0 || height < 0)
		return;
	if (scaled_block_size <= 0)
		return;

	texture *texture;
	block *b;
	SDL_Rect src_rect;
	SDL_Rect dest_rect = {0, 0, scaled_block_size, scaled_block_size};

	const int scaled_view_width = view.screen_width / view.scale;
	const int scaled_view_height = view.screen_height / view.scale;

	const int start_block_x = (int)(((view.x - scaled_view_width / 2) / scaled_block_size) - 1);
	const int start_block_y = (int)(((view.y - scaled_view_height / 2) / scaled_block_size) - 1);

	const int end_block_x = start_block_x + width + 1;
	const int end_block_y = start_block_y + height + 1;

	int frame_x = 0, frame_y = 0;

	// loop over blocks

	// calculate x coordinate of block on screen
	dest_rect.x = start_block_x * scaled_block_size + scaled_view_width / 2 - view.x;

	for (int i = start_block_x; i < end_block_x; i++)
	{
		dest_rect.x += scaled_block_size;
		dest_rect.y = start_block_y * scaled_block_size + scaled_view_height / 2 - view.y;

		for (int j = start_block_y; j < end_block_y; j++)
		{
			dest_rect.y += scaled_block_size;
			// calculate y coordinate of block on screen

			// get block
			if (!(b = get_block_access(w, layer_index, i, j)))
				continue;
			// check if block is not void
			if (b->id == 0)
				continue;
			// check if block is not from the same registry
			if (b->id != b_res[b->id].block_sample.id)
				continue;
			// get texture
			if (!(texture = &b_res[b->id].block_texture))
				continue;
			// get frame
			frame_x = frame_y = 0;
			if (texture->frames > 1)
			{
				int frame = frame_number % texture->frames;
				frame_x = texture->frame_side_size * (frame % texture->frames_per_line);
				frame_y = texture->frame_side_size * (frame / texture->frames_per_line);
			}
			// source is for from what part of texture render
			src_rect.x = frame_x;
			src_rect.y = frame_y;
			src_rect.h = texture->frame_side_size;
			src_rect.w = texture->frame_side_size;
			// and dest_rect is for where on window render it
			// SDL_Rect dest_rect = {(int)dest_x, (int)dest_y, (int)scaled_block_size, (int)scaled_block_size};
			// no camera rotating, yet...
			SDL_RenderCopy(g_renderer, texture->ptr, &src_rect, &dest_rect);
			// printf("rendering block %d at %d, %d\n", b->id, i, j);
		}
	}
	// also draw red square around block at camera position
	SDL_SetRenderDrawColor(g_renderer, 255, 0, 0, 255);
	SDL_Rect rect = {
		view.screen_width / 2 + scaled_block_size / 2,
		view.screen_height / 2 + scaled_block_size / 2,
		scaled_block_size,
		scaled_block_size};
	SDL_RenderDrawRect(g_renderer, &rect);
	SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
}

// renders single frame of the world.
void client_render(const world *w, block_resources *b_res, client_view_point view_point, const int frame_number)
{
	// size of single block in pixels
	const float scaled_block_size = (block_size * view_point.scale);
	// calculate how much blocks to render
	const int blocks_to_render_width = view_point.screen_width / scaled_block_size + 2;
	const int block_to_render_height = view_point.screen_height / scaled_block_size + 2;

	for (int i = 0; i < view_point.draw_order.length; i++)
		layer_render(w, view_point.draw_order.data[i], b_res, frame_number,
					 blocks_to_render_width,
					 block_to_render_height,
					 scaled_block_size,
					 view_point);
}

// TODO: after handling block updates from server, make sure to write function that rerenders only changed parts of the world.
// until then, just rerender everything.
