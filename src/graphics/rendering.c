#include "include/rendering.h"
#include "include/block_renderer_v2.h"
#include "include/flags.h"
#include "include/general.h"
#include "include/level.h"
#include "include/sdl2_basics.h"
#include "include/spatial_grid.h"
#include "include/vars.h"

#define BLOCKS_EXTRA 1

const unsigned int funny_primes[] = {1155501, 6796373, 7883621, 4853063, 8858313,
									 6307353, 1532671, 6233633, 873473,	 685613};
const u8 funny_shifts[] = {9, 7, 5, 3, 1, 2, 4, 6, 8, 10};

static unsigned short tile_rand(const int x, const int y)
{
	int prime = x % 10;
	int antiprime = 9 - (y % 10);
	int midprime = (prime + antiprime) / 2;
	return ((funny_primes[prime] + funny_shifts[antiprime] * funny_primes[midprime]) >> funny_shifts[prime]) & 0x7fff;
}

const float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

static u8 autotile_table_type_1[] = {4, 4, 4, 0, 4, 4, 2, 1, 4, 6, 4, 3, 8, 7, 5, 4};
static u8 autotile_table_type_2[] = {15, 12, 3, 0, 14, 13, 2, 1, 11, 8, 7, 4, 10, 9, 6, 5};

static u8 autotile_select_shared_9(const layer *l, const u8 select_table[], const u64 ref_id, const i32 x, const i32 y)
{
	bool match_west = false;
	bool match_east = false;
	bool match_north = false;
	bool match_south = false;

	if (x > 0 && x < l->width - 1 && y >= 0)
	{
		match_west = *BLOCK_ID_PTR(l, x - 1, y) == ref_id;
		match_east = *BLOCK_ID_PTR(l, x + 1, y) == ref_id;
	}
	if (y > 0 && y < l->height - 1 && x >= 0)
	{
		match_north = *BLOCK_ID_PTR(l, x, y - 1) == ref_id;
		match_south = *BLOCK_ID_PTR(l, x, y + 1) == ref_id;
	}

	return select_table[(match_east & 1) | ((match_south & 1) << 1) | ((match_west & 1) << 2) |
						((match_north & 1) << 3)];
}

static u8 autotile_table_type_3[] = {
	[0b00000010] = 0,  [0b00001010] = 1,  [0b00011010] = 2,	 [0b00010010] = 3,	[0b11011010] = 4,  [0b00011011] = 5,
	[0b00011110] = 6,  [0b01111010] = 7,  [0b00001011] = 8,	 [0b01011111] = 9,	[0b00011111] = 10, [0b00010110] = 11,
	[0b01000010] = 12, [0b01001010] = 13, [0b01011010] = 14, [0b01010010] = 15, [0b01001011] = 16, [0b01111111] = 17,
	[0b11011111] = 18, [0b01010110] = 19, [0b01101011] = 20, [0b01111110] = 21, [0b10000000] = 22, [0b11011110] = 23,
	[0b01000000] = 24, [0b01001000] = 25, [0b01011000] = 26, [0b01010000] = 27, [0b01101010] = 28, [0b11111011] = 29,
	[0b11111110] = 30, [0b11010010] = 31, [0b01111011] = 32, [0b11111111] = 33, [0b11011011] = 34, [0b11010110] = 35,
	[0b00000000] = 36, [0b00001000] = 37, [0b00011000] = 38, [0b00010000] = 39, [0b01011110] = 40, [0b01111000] = 41,
	[0b11011000] = 42, [0b01011011] = 43, [0b01101000] = 44, [0b11111000] = 45, [0b11111010] = 46, [0b11010000] = 47,
};

static bool block_matches(const layer *l, const u64 ref_id, const i32 x, const i32 y)
{
	if (x < 0 || x >= l->width)
		return false;
	if (y < 0 || y >= l->height)
		return false;
	return *BLOCK_ID_PTR(l, x, y) == ref_id;
}

#define BITN(n) (1 << (7 - n))
#define IS_BITN_TRUE(mask, n) (mask & BITN(n)) != 0
#define IS_BITN_FALSE(mask, n) (mask & BITN(n)) == 0

static void invalidate_bit_edge(u8 *in, const u8 nc, const u8 n1, const u8 n2)
{
	u16 bitmap = *in;
	if (IS_BITN_FALSE(bitmap, nc))
		return;
	if ((IS_BITN_TRUE(bitmap, n1) && IS_BITN_TRUE(bitmap, n2)))
		return;
	FLAG_FLIP(bitmap, BITN(nc));
	*in = bitmap;
}

static u8 invalidate_edges(u8 bitmap)
{

	invalidate_bit_edge(&bitmap, 0, 1, 3);
	invalidate_bit_edge(&bitmap, 2, 1, 4);
	invalidate_bit_edge(&bitmap, 5, 3, 6);
	invalidate_bit_edge(&bitmap, 7, 6, 4);

	return bitmap;

#undef IS_BITN_TRUE
#undef IS_BITN_FALSE
#undef BITN
}

static u8 autotile_select_shared_47(const layer *l, const u8 select_table[], const u64 ref_id, const i32 x, const i32 y)
{
#define MATCH(num) (match##num << (7 - num))
	bool match0 = block_matches(l, ref_id, x - 1, y - 1) ? 1 : 0;
	bool match1 = block_matches(l, ref_id, x, y - 1) ? 1 : 0;
	bool match2 = block_matches(l, ref_id, x + 1, y - 1) ? 1 : 0;
	bool match3 = block_matches(l, ref_id, x - 1, y) ? 1 : 0;

	bool match4 = block_matches(l, ref_id, x + 1, y) ? 1 : 0;
	bool match5 = block_matches(l, ref_id, x - 1, y + 1) ? 1 : 0;
	bool match6 = block_matches(l, ref_id, x, y + 1) ? 1 : 0;
	bool match7 = block_matches(l, ref_id, x + 1, y + 1) ? 1 : 0;

	u8 bitmap = MATCH(0) | MATCH(1) | MATCH(2) | MATCH(3) | //
				MATCH(4) | MATCH(5) | MATCH(6) | MATCH(7);

	u8 index = invalidate_edges(bitmap);
	return select_table[index];
#undef MATCH
}

typedef struct
{
	layer_slice slice;
	block_registry *b_reg;
	layer *l;
	int local_block_width;
	i32 block_x_offset;
	i32 block_y_offset;
	i32 start_block_x;
	i32 start_block_y;
	u32 ms_since_start;
	float seconds_since_start;
} render_context;

static void render_block_callback(void *ctx, u16 x, u16 y, u8 *cached_frame)
{
	render_context *rc = (render_context *)ctx;

	i32 i = (i32)x;
	i32 j = (i32)y;

	layer *l = rc->l;
	const u8 bytes_per_block = l->total_bytes_per_block;
	const int local_block_width = rc->local_block_width;

	i32 dest_x =
		-rc->block_x_offset - local_block_width * (BLOCKS_EXTRA + 1) + (i - rc->start_block_x + 1) * local_block_width;
	i32 dest_y =
		-rc->block_y_offset - local_block_width * (BLOCKS_EXTRA + 1) + (j - rc->start_block_y + 1) * local_block_width;

	u64 id = *(l->blocks + ((j * l->width) + i) * bytes_per_block);

	if (id == 0)
		return;

	block_resources br = rc->b_reg->resources.data[id];

	u8 frame = 0;
	u8 type = 0;
	u8 flip = 0;
	i16 rotation = 0;
	i16 offset_x = 0;
	i16 offset_y = 0;

	blob *var = NULL;
	u32 ms_started_moving = rc->slice.timestamp_old;

	if (block_get_vars(l, i, j, &var) == SUCCESS)
	{
		if (br.type_controller != 0)
			var_get_u8_fast(*var, br.type_controller, br.vars_offsets, &type);

		if (br.flip_controller != 0)
			var_get_u8_fast(*var, br.flip_controller, br.vars_offsets, &flip);

		if (br.rotation_controller != 0)
			var_get_i16_fast(*var, br.rotation_controller, br.vars_offsets, &rotation);

		if (br.anim_controller != 0)
			var_get_u8_fast(*var, br.anim_controller, br.vars_offsets, &frame);

		if (br.offset_x_controller != 0)
			var_get_i16_fast(*var, br.offset_x_controller, br.vars_offsets, &offset_x);

		if (br.offset_y_controller != 0)
			var_get_i16_fast(*var, br.offset_y_controller, br.vars_offsets, &offset_y);

		if (br.interp_takes != 0 && br.interp_timestamp_controller != 0)
		{
			var_get_u32_fast(*var, br.interp_timestamp_controller, br.vars_offsets, &ms_started_moving);

			const float pos = (rc->ms_since_start - ms_started_moving) / (float)br.interp_takes;
			const float clamp_pos = fmax(0.0f, fmin(1.0, pos));

			offset_x = (i16)lerp((float)offset_x, 0, clamp_pos);
			offset_y = (i16)lerp((float)offset_y, 0, clamp_pos);
		}
	}

	if (br.autotile_type == 1)
	{
		if (*cached_frame != AUTOTILE_CACHE_INVALID)
			frame = *cached_frame;
		else
		{
			frame = autotile_select_shared_9(l, autotile_table_type_1, id, i, j);
			*cached_frame = frame;
		}
	}
	else if (br.autotile_type == 2)
	{
		if (*cached_frame != AUTOTILE_CACHE_INVALID)
			frame = *cached_frame;
		else
		{
			frame = autotile_select_shared_9(l, autotile_table_type_2, id, i, j);
			*cached_frame = frame;
		}
	}
	else if (br.autotile_type == 3)
	{
		if (*cached_frame != AUTOTILE_CACHE_INVALID)
			frame = *cached_frame;
		else
		{
			frame = autotile_select_shared_47(l, autotile_table_type_3, id, i, j);
			*cached_frame = frame;
		}
	}

	if (br.frames_per_second > 1)
	{
		int fps = br.frames_per_second;
		frame = (u8)(rc->seconds_since_start * fps);
	}

	if (FLAG_GET(br.flags, RESOURCE_FLAG_RANDOM_POS))
	{
		frame = tile_rand(i, j) % br.info.total_frames;
	}

	if (br.override_frame != 0)
		frame = br.override_frame;

	if (frame || type || flip || rotation)
	{
		u8 actual_frame = frame % br.info.frames;
		u8 actual_type =
			FLAG_GET(br.flags, RESOURCE_FLAG_IGNORE_TYPE) ? (u8)(frame / br.info.frames) : (type % br.info.types);
		renderer_v2_add_instance(dest_x + offset_x, dest_y + offset_y, br.info.atlas_offset_x + actual_frame,
								 br.info.atlas_offset_y + actual_type, flip, rc->slice.zoom * 1.0f,
								 rc->slice.zoom * 1.0f, (float)rotation * (M_PI / 180.0f));
	}
	else
	{
		renderer_v2_add_instance(dest_x + offset_x, dest_y + offset_y, br.info.atlas_offset_x, br.info.atlas_offset_y,
								 0, rc->slice.zoom * 1.0f, rc->slice.zoom * 1.0f, 0);
	}
}

u8 render_layer(layer_slice slice)
{
	assert(slice.zoom > 0);
	const int local_block_width = (g_block_width * slice.zoom);

	const int width = slice.w / local_block_width;
	const int height = slice.h / local_block_width;

	if (!slice.ref)
		return SUCCESS;

	if (!slice.ref->blocks)
		return SUCCESS;

	block_registry *b_reg = slice.ref->registry;

	const u32 ms_since_start = SDL_GetTicks();
	// const u32 ms_since_start = clock();
	const float seconds_since_start = ms_since_start / 1000.0f;

	u32 ms_started_moving = slice.timestamp_old;

	const float slice_clamp_pos = fmax(0.0f, fmin(1.0, (ms_since_start - ms_started_moving) / (1000.0f / TPS)));

	slice.x = lerp(slice.old_x, slice.x, slice_clamp_pos);
	slice.y = lerp(slice.old_y, slice.y, slice_clamp_pos);

	const i32 start_block_x = ((slice.x / local_block_width) - BLOCKS_EXTRA);
	const i32 start_block_y = ((slice.y / local_block_width) - BLOCKS_EXTRA);

	const i32 end_block_x = start_block_x + width + BLOCKS_EXTRA + 1;
	const i32 end_block_y = start_block_y + height + BLOCKS_EXTRA + 1;

	const i32 block_x_offset = slice.x % local_block_width;
	const i32 block_y_offset = slice.y % local_block_width;

	GLuint texture = b_reg->atlas_texture_uid;

	renderer_v2_begin_batch(texture, b_reg->atlas, g_block_width);

	layer *l = (layer *)slice.ref;

	if (l->spatial.cells == NULL)
	{
		spatial_grid_build_from_layer(&l->spatial, l->width, l->height, l->block_size, l->blocks,
									  l->total_bytes_per_block);
	}

	render_context rc = {
		.slice = slice,
		.b_reg = b_reg,
		.l = l,
		.local_block_width = local_block_width,
		.block_x_offset = block_x_offset,
		.block_y_offset = block_y_offset,
		.start_block_x = start_block_x,
		.start_block_y = start_block_y,
		.ms_since_start = ms_since_start,
		.seconds_since_start = seconds_since_start,
	};

	spatial_grid_get_visible((spatial_grid *)&l->spatial, start_block_x, start_block_y, end_block_x, end_block_y, &rc,
							 render_block_callback);

	renderer_v2_end_batch();

	return SUCCESS;
}

// u8 render_layer_to_framebuffer(layer_slice *slice)
// {
//     layer *l = slice->ref;

//     u32 width = l->width * g_block_width;
//     u32 height = l->height * g_block_width;

//     framebuffer fb = slice->framebuffer == 0 || slice->framebuffer_texture == 0
//                          ? create_framebuffer_object(width, height)
//                          : (framebuffer){
//                                .fbo = slice->framebuffer,
//                                .texture = slice->framebuffer_texture,
//                            };

//     if (fb.fbo == 0 || fb.texture == 0)
//     {
//         LOG_ERROR("Failed to create framebuffer");
//         return FAIL;
//     }

//     glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

//     // Create a fresh slice for the full layer render
//     layer_slice full_slice = *slice;
//     full_slice.x = 0;
//     full_slice.y = 0;
//     full_slice.w = width;
//     full_slice.h = height;
//     full_slice.zoom = 1;
//     full_slice.flags = 0;

//     render_layer(full_slice);

//     (*slice).framebuffer = fb.fbo;
//     (*slice).framebuffer_texture = fb.texture;

//     FLAG_SET((*slice).flags, LAYER_SLICE_FLAG_RENDER_COMPLETE, 1);

//     return SUCCESS;
// }

u8 client_render(const client_render_rules rules)
{
	renderer_v2_begin_frame();

	for (u32 i = 0; i < rules.draw_order.length; i++)
	{
		int layer_id = rules.draw_order.data[i];
		layer_slice slice = rules.slices.data[layer_id];

		render_layer(slice);
	}

	renderer_v2_end_frame();

	return SUCCESS;
}