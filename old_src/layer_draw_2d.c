#include "include/layer_draw_2d.h"
#include "include/data_manipulations.h"
#include "include/world_utils.h"

// 45801564169

const unsigned int funny_primes[] = {1155501, 6796373, 7883621, 4853063, 8858313, 6307353, 1532671, 6233633, 873473, 685613};
const byte funny_shifts[] = {9, 7, 5, 3, 1, 2, 4, 6, 8, 10};

static unsigned short
tile_rand(const int x, const int y)
{
	int prime = x % 10;
	int antiprime = 9 - (y % 10);
	int midprime = (prime + antiprime) / 2;
	return ((funny_primes[prime] + funny_shifts[antiprime] * funny_primes[midprime]) >> funny_shifts[prime]) &
		   0x7fff;
}

// renders layer of the world. smart enough to not render what player doesnt see
void layer_render(const world *w, const int layer_index, block_resources_t *b_reg,
				  const int frame_number,
				  const layer_slice slice)
{
	if (!w || !b_reg)
		return;
	if (layer_index < 0 || layer_index >= w->layers.length)
		return;

	const int local_block_width = (g_block_width * slice.mult);

	const int width = slice.w / local_block_width; // exact amowunt of lboks to render o nscren
	const int height = slice.h / local_block_width;

	texture *texture;
	block *b;

	const int start_block_x = ((slice.x / local_block_width) - 1);
	const int start_block_y = ((slice.y / local_block_width) - 1);

	const int end_block_x = start_block_x + width + 2;
	const int end_block_y = start_block_y + height + 2;
	// create a chunk segment

	const int chunk_width = w->layers.data[layer_index].chunk_width;

	chunk_segment cs = chunk_segment_create(w, layer_index,
											start_block_x / chunk_width,
											start_block_y / chunk_width,
											1 + end_block_x / chunk_width,
											1 + end_block_y / chunk_width);

	// if (layer_index == 0)
	// {
	// 	bprintf(w, b_reg, 0, 0, 0, 32, "%d    %d    ", start_block_x, start_block_y);
	// 	bprintf(w, b_reg, 0, 0, 3, 32, "%d    %d    ", end_block_x, end_block_y);
	// 	bprintf(w, b_reg, 0, 0, 1, 32, "%d    %d    ", slice.x, slice.y);
	// 	bprintf(w, b_reg, 0, 0, 2, 32, "%d    %d    ", slice.w, slice.h);
	// 	bprintf(w, b_reg, 0, 0, 4, 32, "%d    %d    %d    %d    ", start_block_x / chunk_width, start_block_y / chunk_width, 1 + end_block_x / chunk_width, 1 + end_block_y / chunk_width);
	// }

	const int block_x_offset = slice.x % local_block_width; /* offset in pixels for smooth rendering of blocks */
	const int block_y_offset = slice.y % local_block_width;

	float dest_x, dest_y;
	dest_x = -block_x_offset - local_block_width * 2; // also minus 1 full block back to fill the gap

	// if (layer_index == 0)
	// {
	// 	bprintf(w, b_reg, 0, 0, 2, 32, "start coords: %d    %d    ", -block_x_offset - local_block_width, -block_y_offset - local_block_width);
	// }

	for (int i = start_block_x; i < end_block_x; i++)
	{
		dest_x += local_block_width;
		dest_y = -block_y_offset - local_block_width * 2;

		for (int j = start_block_y; j < end_block_y; j++)
		{
			dest_y += local_block_width;
			// calculate y coordinate of block on screen

			// get block
			if (!(b = chunk_segment_get_block_access(cs, i, j)))
				continue;
			// check if block is not void
			if (b->id == 0)
				continue;
			// check if block could exist in registry
			if (b->id >= b_reg->length)
				continue;

			block_resources br = b_reg->data[b->id];

			// get texture
			texture = &br.block_texture;
			if (!texture)
				continue;

			byte frame = 0;
			byte type = 0;
			byte flip = 0;
			unsigned short rotation = 0;

			if (br.type_controller != 0)
			{
				(void)data_get_b(b, br.type_controller, &type);
			}

			if (br.flip_controller != 0)
			{
				(void)data_get_b(b, br.flip_controller, &flip);
			}

			if (br.rotation_controller != 0)
			{
				//(void)data_get_b(b, br.rotation_controller, &rotation);
				data_get_number(b, br.rotation_controller, (long long *)&rotation);
			}

			if (br.anim_controller != 0)
			{
				(void)data_get_b(b, br.anim_controller, &frame);
			}
			else if (br.frames_per_second > 1)
			{
				float seconds_since_start = SDL_GetTicks() / 1000.0f;
				int fps = br.frames_per_second;
				frame = (byte)(seconds_since_start * fps);
			}
			else if (FLAG_GET(br.flags, B_RES_FLAG_RANDOM_POS))
			{
				frame = tile_rand(i, j);
			}

			block_render(texture, dest_x, dest_y, frame, type, FLAG_GET(br.flags, B_RES_FLAG_IGNORE_TYPE), local_block_width, flip, rotation);
		}
	}

	chunk_segment_free(&cs);
}

// renders single frame of the world.
void client_render(const world *w, block_resources_t *b_reg, client_render_rules render_rules, const int frame_number)
{
	for (int i = 0; i < render_rules.draw_order.length; i++)
	{
		int layer_id = render_rules.draw_order.data[i];
		layer_render(w, layer_id, b_reg, frame_number,
					 render_rules.slices.data[layer_id]);
	}
}

// TODO: after handling block updates from server, make sure to write function that rerenders only changed parts of the world.
// until then, just rerender everything.
