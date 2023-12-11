#include "sdl2_basics.c"
#include "block_registry.c"
#include "block_updates.c"

// view point relative to
SDL_Point view_point = {0, 0};
const int block_size = 16;

// renders layer of the world. smart enough to not render what player doesnt see
void layer_render(const world *w, const int layer_index, const block_resources *b_res, const float scale, const int frame_number)
{
	const float scaled_block_size = (block_size * scale);
	const int block_begin_x = (int)(view_point.x - SCREEN_WIDTH / 2) / scaled_block_size - 1;  // adding and substracting block
	const int block_begin_y = (int)(view_point.y - SCREEN_HEIGHT / 2) / scaled_block_size - 1; // coordinates to render
	const int block_end_x = (int)(view_point.x + SCREEN_WIDTH / 2) / scaled_block_size + 1;	   // extra blocks at the corners
	const int block_end_y = (int)(view_point.y + SCREEN_HEIGHT / 2) / scaled_block_size + 1;

	float dest_x = 0;
	texture *block_texture;
	block *b;
	for (int i = block_begin_x; i < block_end_x; i++, dest_x += scaled_block_size)
	{
		float dest_y = 0;
		for (int j = block_begin_y; j < block_end_y; j++, dest_y += scaled_block_size)
		{
			if (!(b = get_block_access(w, layer_index, i, j))) // no blocks? fine
			{
				j += j % w->layers[layer_index].chunk_width - 1;
				continue;
			}
			if (b->id == 0) // void block? fine
				continue;
			if (!(block_texture = &b_res[b->id].block_texture)) // unlikely, but no texture? fine...
				continue;

			int frame_x = 0, frame_y = 0;
			// if 1 frame, just render as it is
			if (block_texture->frames > 1)
			{
				int frame = frame_number % block_texture->frames;
				frame_x = block_texture->frame_side_size * (frame % block_texture->frames_per_line);
				frame_y = block_texture->frame_side_size * (frame / block_texture->frames_per_line);
			}
			// source is for from what part of texture render
			SDL_Rect src = {frame_x, frame_y, block_texture->frame_side_size, block_texture->frame_side_size};
			// and dest is for where on window render it
			SDL_Rect dest = {(int)dest_x, (int)dest_y, (int)scaled_block_size, (int)scaled_block_size};
			// no camera rotating, yet...
			SDL_RenderCopy(g_renderer, block_texture->ptr, &src, &dest);
		}
	}
}

/*

plan for rendering just 1 frame

1) update world and save changes
2) re-render blocks that changed
3) re-render blocks with animation (if needed)
4)

???





???
calculate graphic delta:
	1) take layer data
	2) take block registry vector
	3) take view point
	4) take previous layer block map

	5) calculate, which block needs rerender
	6)




1) take layer data
2) take block registry vector
3) take view point, {x, y, scale}
4) calculate rectangle with blocks that player can see
5) draw every block with coresponding block_texture and animtion rules (if block have any) in that rectangle
6) exit

*/