#include "include/rendering.h"
#include "include/block_entity.h"
#include "include/block_registry.h"
#include "include/block_renderer_v2.h"
#include "include/config.h"
#include "include/flags.h"
#include "include/general.h"
#include "include/level.h"
#include "include/logging.h"
#include "include/sdl2_basics.h"
#include "include/spatial_grid.h"
#include "include/vars.h"
#include "include/vec_math.h"
#include <box2d/box2d.h>
#include <box2d/math_functions.h>

#define REGION_MARGIN 1

const u32 funny_primes[] = {1155501, 6796373, 7883621, 4853063, 8858313, 6307353, 1532671, 6233633, 873473, 685613};
const u8 funny_shifts[] = {9, 7, 5, 3, 1, 2, 4, 6, 8, 10};

static unsigned short tile_rand(const i32 x, const i32 y)
{
	i32 prime = x % 10;
	i32 antiprime = 9 - (y % 10);
	i32 midprime = (prime + antiprime) / 2;
	return ((funny_primes[prime] + funny_shifts[antiprime] * funny_primes[midprime]) >> funny_shifts[prime]) & 0x7fff;
}

const f32 lerp(f32 a, f32 b, f32 f)
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
	u8 frame;
	u8 type;
	u8 flip;
	i16 rotation;
	i16 offset_x;
	i16 offset_y;
} computed_render_props;

static void compute_render_props(const block_resources *br, blob *var, u32 ms_since_start, u32 default_timestamp,
								 i16 base_rotation, computed_render_props *out)
{
	out->frame = 0;
	out->type = 0;
	out->flip = 0;
	out->rotation = base_rotation;
	out->offset_x = 0;
	out->offset_y = 0;

	u32 ms_started_moving = default_timestamp;
	f32 seconds_since_start = ms_since_start / 1000.0f;

	if (var)
	{
		if (br->type_controller != 0)
			var_get_u8_fast(*var, br->type_controller, br->vars_offsets, &out->type);

		if (br->flip_controller != 0)
			var_get_u8_fast(*var, br->flip_controller, br->vars_offsets, &out->flip);

		if (br->rotation_controller != 0)
			var_get_i16_fast(*var, br->rotation_controller, br->vars_offsets, &out->rotation);

		if (br->anim_controller != 0)
			var_get_u8_fast(*var, br->anim_controller, br->vars_offsets, &out->frame);

		if (br->offset_x_controller != 0)
			var_get_i16_fast(*var, br->offset_x_controller, br->vars_offsets, &out->offset_x);

		if (br->offset_y_controller != 0)
			var_get_i16_fast(*var, br->offset_y_controller, br->vars_offsets, &out->offset_y);

		if (br->interp_takes != 0 && br->interp_timestamp_controller != 0)
		{
			var_get_u32_fast(*var, br->interp_timestamp_controller, br->vars_offsets, &ms_started_moving);

			const f32 pos = (ms_since_start - ms_started_moving) / (f32)br->interp_takes;
			const f32 clamp_pos = fmax(0.0f, fmin(1.0, pos));

			out->offset_x = (i16)lerp((f32)out->offset_x, 0, clamp_pos);
			out->offset_y = (i16)lerp((f32)out->offset_y, 0, clamp_pos);
		}
	}

	if (br->frames_per_second > 1)
	{
		i32 fps = br->frames_per_second;
		out->frame = (u8)(seconds_since_start * fps);
	}

	if (br->override_frame != 0)
		out->frame = br->override_frame;
}

//
// Per-layer world-space slot cache.
//
// Each layer keeps a dense slot array covering the visible block region plus a
// margin. Slots are moved/exposed when the camera slides, rebuilt on block
// writes (layer render_version), and revalidated per frame only when the
// resource can change appearance through var controllers.
//

typedef enum
{
	RENDER_SLOT_VOID = 0,
	RENDER_SLOT_STATIC,
	RENDER_SLOT_VAR,
	RENDER_SLOT_TIME,
} render_slot_kind;

typedef struct
{
	u8 kind;
	u8 needs_rebuild;
	u8 needs_upload;
	u8 valid;
} render_slot_meta;

typedef struct layer_render_cache
{
	u32 version_seen;
	f32 zoom_seen;
	i32 region_bx0, region_by0;
	u32 region_bw, region_bh;

	instance_data *slot_insts;
	render_slot_meta *slot_meta;
	instance_data *inst_scratch;
	render_slot_meta *meta_scratch;
	u32 slot_capacity;

	instance_data *entity_data;
	u32 entity_capacity;
	u32 entity_count;

	layer_batch batch;
} layer_render_cache;

static u64 layer_read_id(const layer *l, i32 bx, i32 by)
{
	u8 *ptr = BLOCK_ID_PTR(l, bx, by);
	u64 id = 0;
	switch (l->block_size)
	{
	case 1:
		id = *(u8 *)ptr;
		break;
	case 2:
		id = *(u16 *)ptr;
		break;
	case 4:
		id = *(u32 *)ptr;
		break;
	case 8:
		id = *(u64 *)ptr;
		break;
	default:
		assert(0 && "unsupported block size");
		break;
	}
	return id;
}

static u8 render_slot_classify(const block_resources *br)
{
	if (br->frames_per_second > 1)
		return RENDER_SLOT_TIME;
	if (br->interp_takes != 0 && br->interp_timestamp_controller != 0)
		return RENDER_SLOT_TIME;
	if (br->anim_controller != 0 || br->type_controller != 0 || br->flip_controller != 0 ||
		br->rotation_controller != 0 || br->offset_x_controller != 0 || br->offset_y_controller != 0)
		return RENDER_SLOT_VAR;
	return RENDER_SLOT_STATIC;
}

static void slot_make_void(layer_render_cache *c, u32 n)
{
	instance_data hole = {0};
	render_slot_meta *m = &c->slot_meta[n];
	if (m->kind != RENDER_SLOT_VOID || m->valid || m->needs_upload ||
		memcmp(&c->slot_insts[n], &hole, sizeof(instance_data)) != 0)
	{
		c->slot_insts[n] = hole;
		m->kind = RENDER_SLOT_VOID;
		m->valid = 0;
		m->needs_upload = 1;
	}
	m->needs_rebuild = 0;
}

static bool compute_slot_instance(instance_data *out, const layer *l, const block_resources *br, i32 bx, i32 by,
								  u32 ms, u32 default_timestamp, f32 view_zoom)
{
	blob *var = NULL;
	block_get_vars(l, (u16)bx, (u16)by, &var);

	computed_render_props props;
	compute_render_props(br, var, ms, default_timestamp, 0, &props);

	if (br->autotile_type)
	{
		u8 frame = spatial_grid_read_cached_frame((spatial_grid *)&l->spatial, (u16)bx, (u16)by);
		if (frame == AUTOTILE_CACHE_INVALID)
		{
			u64 id = layer_read_id(l, bx, by);
			switch (br->autotile_type)
			{
			case 1:
				props.frame = autotile_select_shared_9(l, autotile_table_type_1, id, bx, by);
				break;
			case 2:
				props.frame = autotile_select_shared_9(l, autotile_table_type_2, id, bx, by);
				break;
			case 3:
				props.frame = autotile_select_shared_47(l, autotile_table_type_3, id, bx, by);
				break;
			default:
				break;
			}
			spatial_grid_set_cached_frame((spatial_grid *)&l->spatial, (u16)bx, (u16)by, props.frame);
		}
		else
		{
			props.frame = frame;
		}
	}

	if (FLAG_GET(br->flags, RESOURCE_FLAG_RANDOM_POS))
		props.frame = tile_rand(bx, by) % br->info.total_frames;

	instance_data inst = {0};
	inst.x = (f32)bx * g_block_width + (f32)props.offset_x / view_zoom;
	inst.y = (f32)by * g_block_width + (f32)props.offset_y / view_zoom;
	inst.scale_x = 1.0f;
	inst.scale_y = 1.0f;
	inst.rotation = (f32)props.rotation * (M_PI / 180.0f);
	inst.frame = br->info.atlas_offset_x + (u8)(props.frame % br->info.frames);
	inst.type = FLAG_GET(br->flags, RESOURCE_FLAG_IGNORE_TYPE)
					? (u8)(props.frame / br->info.frames)
					: (u8)(props.type % br->info.types);
	inst.type += br->info.atlas_offset_y;
	inst.flags = props.flip;
	inst.padding = 0;

	*out = inst;
	return true;
}

static layer_render_cache *layer_get_render_cache(layer *l)
{
	layer_render_cache *c = (layer_render_cache *)l->render_cache;
	if (!c)
	{
		c = (layer_render_cache *)calloc(1, sizeof(layer_render_cache));
		l->render_cache = c;
	}
	return c;
}

void layer_free_render_cache(layer *l)
{
	if (!l)
		return;

	layer_render_cache *c = (layer_render_cache *)l->render_cache;
	if (!c)
		return;

	if (renderer_v2.initialized)
	{
		if (c->batch.vao)
			glDeleteVertexArrays(1, &c->batch.vao);
		if (c->batch.instance_vbo)
			glDeleteBuffers(1, &c->batch.instance_vbo);
	}
	SAFE_FREE(c->slot_insts);
	SAFE_FREE(c->slot_meta);
	SAFE_FREE(c->inst_scratch);
	SAFE_FREE(c->meta_scratch);
	SAFE_FREE(c->entity_data);
	SAFE_FREE(c);
	l->render_cache = NULL;
}

static void cache_ensure_region(layer_render_cache *c, u32 bw, u32 bh)
{
	u32 need = bw * bh;
	if (need <= c->slot_capacity)
		return;

	u32 ncap = c->slot_capacity ? c->slot_capacity : 256;
	while (ncap < need)
		ncap *= 2;

	instance_data *ni = (instance_data *)realloc(c->slot_insts, ncap * sizeof(instance_data));
	render_slot_meta *nm = (render_slot_meta *)realloc(c->slot_meta, ncap * sizeof(render_slot_meta));
	instance_data *nis = (instance_data *)realloc(c->inst_scratch, ncap * sizeof(instance_data));
	render_slot_meta *nms = (render_slot_meta *)realloc(c->meta_scratch, ncap * sizeof(render_slot_meta));
	if (!ni || !nm || !nis || !nms)
	{
		LOG_ERROR("failed to grow layer render cache to %u slots", ncap);
		SAFE_FREE(ni);
		SAFE_FREE(nm);
		SAFE_FREE(nis);
		SAFE_FREE(nms);
		return;
	}
	memset(ni + c->slot_capacity, 0, (ncap - c->slot_capacity) * sizeof(instance_data));
	memset(nm + c->slot_capacity, 0, (ncap - c->slot_capacity) * sizeof(render_slot_meta));
	memset(nis + c->slot_capacity, 0, (ncap - c->slot_capacity) * sizeof(instance_data));
	memset(nms + c->slot_capacity, 0, (ncap - c->slot_capacity) * sizeof(render_slot_meta));
	c->slot_insts = ni;
	c->slot_meta = nm;
	c->inst_scratch = nis;
	c->meta_scratch = nms;
	c->slot_capacity = ncap;
}

static void cache_prepare_region(layer_render_cache *c, layer *l, f32 cam_x, f32 cam_y, f32 view_zoom, f32 view_w,
								 f32 view_h)
{
	f32 world_scale = view_zoom * g_block_width;
	i32 bx0 = (i32)floorf(cam_x / world_scale) - REGION_MARGIN;
	i32 by0 = (i32)floorf(cam_y / world_scale) - REGION_MARGIN;
	u32 bw = (u32)ceilf(view_w / world_scale) + 2 * REGION_MARGIN;
	u32 bh = (u32)ceilf(view_h / world_scale) + 2 * REGION_MARGIN;

	cache_ensure_region(c, bw, bh);

	bool geometry_changed = (c->region_bw != bw || c->region_bh != bh || c->zoom_seen != view_zoom);

	if (geometry_changed)
	{
		u32 total = bw * bh;
		memset(c->slot_insts, 0, total * sizeof(instance_data));
		memset(c->slot_meta, 0, total * sizeof(render_slot_meta));
		memset(c->inst_scratch, 0, total * sizeof(instance_data));
		memset(c->meta_scratch, 0, total * sizeof(render_slot_meta));
		c->region_bx0 = bx0;
		c->region_by0 = by0;
		c->region_bw = bw;
		c->region_bh = bh;
		c->zoom_seen = view_zoom;
		c->version_seen = l->render_version;
		for (u32 n = 0; n < total; n++)
		{
			c->slot_meta[n].needs_rebuild = 1;
			c->slot_meta[n].needs_upload = 1;
		}
		return;
	}

	bool rebuild_all = (c->version_seen != l->render_version);
	c->version_seen = l->render_version;

	if (bx0 == c->region_bx0 && by0 == c->region_by0)
	{
		if (rebuild_all)
		{
			for (u32 n = 0; n < bw * bh; n++)
				c->slot_meta[n].needs_rebuild = 1;
		}
		return;
	}

	i32 obx0 = c->region_bx0;
	i32 oby0 = c->region_by0;

	for (u32 sy = 0; sy < bh; sy++)
	{
		for (u32 sx = 0; sx < bw; sx++)
		{
			i32 bx = bx0 + (i32)sx;
			i32 by = by0 + (i32)sy;
			u32 n = sy * bw + sx;

			if (!rebuild_all && bx >= obx0 && bx < obx0 + (i32)bw && by >= oby0 && by < oby0 + (i32)bh)
			{
				u32 o = (u32)(by - oby0) * bw + (u32)(bx - obx0);
				c->inst_scratch[n] = c->slot_insts[o];
				c->meta_scratch[n] = c->slot_meta[o];
				c->meta_scratch[n].needs_upload = 1;
			}
			else
			{
				memset(&c->inst_scratch[n], 0, sizeof(instance_data));
				memset(&c->meta_scratch[n], 0, sizeof(render_slot_meta));
				c->meta_scratch[n].needs_rebuild = 1;
				c->meta_scratch[n].needs_upload = 1;
			}
		}
	}

	instance_data *iswap = c->slot_insts;
	c->slot_insts = c->inst_scratch;
	c->inst_scratch = iswap;

	render_slot_meta *mswap = c->slot_meta;
	c->slot_meta = c->meta_scratch;
	c->meta_scratch = mswap;

	c->region_bx0 = bx0;
	c->region_by0 = by0;
}

static void cache_fill(layer_render_cache *c, layer *l, block_registry *b_reg, u32 ms, u32 default_timestamp,
					   f32 view_zoom)
{
	const u32 bw = c->region_bw, bh = c->region_bh;
	block_resources_t *res = &b_reg->resources;

	for (u32 sy = 0; sy < bh; sy++)
	{
		for (u32 sx = 0; sx < bw; sx++)
		{
			u32 n = sy * bw + sx;
			render_slot_meta *m = &c->slot_meta[n];

			if (!m->needs_rebuild && m->kind == RENDER_SLOT_STATIC)
				continue;

			i32 bx = c->region_bx0 + (i32)sx;
			i32 by = c->region_by0 + (i32)sy;

			if (bx < 0 || by < 0 || bx >= (i32)l->width || by >= (i32)l->height)
			{
				slot_make_void(c, n);
				continue;
			}

			u64 id = layer_read_id(l, bx, by);
			if (id == 0)
			{
				slot_make_void(c, n);
				continue;
			}

			block_resources *br = &res->data[id];
			u8 kind = render_slot_classify(br);

			if (kind == RENDER_SLOT_STATIC && m->kind == RENDER_SLOT_STATIC && !m->needs_rebuild)
				continue;

			instance_data inst;
			compute_slot_instance(&inst, l, br, bx, by, ms, default_timestamp, view_zoom);

			if (memcmp(&c->slot_insts[n], &inst, sizeof(instance_data)) != 0)
				m->needs_upload = 1;
			c->slot_insts[n] = inst;
			m->kind = kind;
			m->valid = 1;
			m->needs_rebuild = 0;
		}
	}
}

typedef struct
{
	layer_render_cache *cache;
	block_registry *b_reg;
	u32 ms;
	f32 zoom;
	f32 cull_x0, cull_y0, cull_x1, cull_y1;
} render_entity_ctx;

static u32 render_entity_cb(handle32 h, void *ptr, void *user_data)
{
	(void)h;
	render_entity_ctx *rc = (render_entity_ctx *)user_data;
	block_entity *e = (block_entity *)ptr;
	if (!e || !e->block_id)
		return SUCCESS;
	if (!b2Body_IsValid(e->b2_body_id))
		return SUCCESS;

	f32 clamp_pos = fmax(0.0f, fmin(1.0, (rc->ms - e->timestamp_old) / (1000.0f / TPS)));

	b2Vec2 e_pos = b2Body_GetPosition(e->b2_body_id);
	f32 rotation = RAD_TO_DEG(b2Rot_GetAngle(b2Body_GetRotation(e->b2_body_id)));

	f32 interp_x = lerp((f32)e->pos_old.x, (f32)e_pos.x, clamp_pos);
	f32 interp_y = lerp((f32)e->pos_old.y, (f32)e_pos.y, clamp_pos);

	if (interp_x < rc->cull_x0 || interp_x > rc->cull_x1 || interp_y < rc->cull_y0 || interp_y > rc->cull_y1)
		return SUCCESS;

	block_resources *br = &rc->b_reg->resources.data[e->block_id];

	blob *var = NULL;
	block_entity_get_vars(e, &var);

	computed_render_props props;
	compute_render_props(br, var, rc->ms, e->timestamp_old, (i16)roundf(rotation), &props);

	f32 cx = interp_x - (e->scale_x * g_block_width) * 0.5f;
	f32 cy = interp_y - (e->scale_y * g_block_width) * 0.5f;

	instance_data inst = {0};
	inst.x = cx + (f32)props.offset_x / rc->zoom;
	inst.y = cy + (f32)props.offset_y / rc->zoom;
	inst.scale_x = e->scale_x;
	inst.scale_y = e->scale_y;
	inst.rotation = (f32)props.rotation * (M_PI / 180.0f);
	inst.flags = props.flip;
	inst.frame = br->info.atlas_offset_x + (u8)(props.frame % br->info.frames);
	inst.type = FLAG_GET(br->flags, RESOURCE_FLAG_IGNORE_TYPE) ? (u8)(props.frame / br->info.frames)
															   : (u8)(props.type % br->info.types);
	inst.type += br->info.atlas_offset_y;
	inst.padding = 0;

	layer_render_cache *c = rc->cache;
	if (c->entity_count >= c->entity_capacity)
	{
		u32 ncap = c->entity_capacity ? c->entity_capacity * 2 : 64;
		instance_data *nd = (instance_data *)realloc(c->entity_data, ncap * sizeof(instance_data));
		if (!nd)
			return FAIL;
		c->entity_data = nd;
		c->entity_capacity = ncap;
	}
	c->entity_data[c->entity_count++] = inst;
	return SUCCESS;
}

static void render_entities(layer_render_cache *c, layer *l, block_registry *b_reg, u32 ms, f32 zoom)
{
	c->entity_count = 0;
	if (!l->block_entity_pool || l->block_entity_count_estimate == 0)
		return;

	f32 wx0 = (f32)c->region_bx0 * g_block_width - g_block_width;
	f32 wy0 = (f32)c->region_by0 * g_block_width - g_block_width;
	f32 wx1 = (f32)(c->region_bx0 + (i32)c->region_bw) * g_block_width + g_block_width;
	f32 wy1 = (f32)(c->region_by0 + (i32)c->region_bh) * g_block_width + g_block_width;

	render_entity_ctx rc = {
		.cache = c,
		.b_reg = b_reg,
		.ms = ms,
		.zoom = zoom,
		.cull_x0 = wx0,
		.cull_y0 = wy0,
		.cull_x1 = wx1,
		.cull_y1 = wy1,
	};

	handle_table_iterate(l->block_entity_pool, render_entity_cb, &rc);
}

static void cache_submit(layer_render_cache *c, GLuint texture, image *atlas, u8 block_width)
{
	renderer_v2_layer_begin(&c->batch, texture, atlas, block_width);

	const u32 total = c->region_bw * c->region_bh;
	u32 i = 0;
	while (i < total)
	{
		if (!c->slot_meta[i].needs_upload)
		{
			i++;
			continue;
		}
		u32 start = i, cnt = 0;
		while (i < total && c->slot_meta[i].needs_upload)
		{
			c->slot_meta[i].needs_upload = 0;
			cnt++;
			i++;
		}
		renderer_v2_layer_upload(&c->batch, start, cnt, &c->slot_insts[start]);
	}

	if (c->entity_count)
		renderer_v2_layer_upload(&c->batch, total, c->entity_count, c->entity_data);

	renderer_v2_layer_draw(&c->batch, total + c->entity_count);
}

u8 render_layer(layer_slice slice)
{
	assert(slice.zoom > 0);
	layer *l = (layer *)slice.ref;
	if (!l || !l->blocks)
		return SUCCESS;

	block_registry *b_reg = l->registry;

	const u32 ms = SDL_GetTicks();

	const bool is_ui = (l->flags & LAYER_FLAG_UI) != 0;

	const u32 interp_takes = slice.interp_takes != 0 ? slice.interp_takes : (1000 / TPS);
	const f32 clamp_pos = fmax(0.0f, fmin(1.0, (f32)(ms - slice.timestamp_old) / (f32)interp_takes));
	f32 cam_x = lerp((f32)slice.old_x, (f32)slice.x, clamp_pos);
	f32 cam_y = lerp((f32)slice.old_y, (f32)slice.y, clamp_pos);
	if (!is_ui)
	{
		if (cam_x < 0.0f)
			cam_x = 0.0f;
		if (cam_y < 0.0f)
			cam_y = 0.0f;
	}

	const f32 view_zoom = slice.zoom > 0.0f ? slice.zoom : 1.0f;
	const f32 proj_cam_x = is_ui ? 0.0f : cam_x;
	const f32 proj_cam_y = is_ui ? 0.0f : cam_y;

	renderer_v2_set_view(slice.w, slice.h, proj_cam_x, proj_cam_y, view_zoom);

	if (l->spatial.cells == NULL)
		spatial_grid_build_from_layer(&l->spatial, l->width, l->height, l->block_size, l->blocks, l->total_bytes_per_block);

	layer_render_cache *c = layer_get_render_cache(l);

	cache_prepare_region(c, l, cam_x, cam_y, view_zoom, (f32)slice.w, (f32)slice.h);
	cache_fill(c, l, b_reg, ms, slice.timestamp_old, view_zoom);
	render_entities(c, l, b_reg, ms, view_zoom);

	cache_submit(c, b_reg->atlas_texture_uid, b_reg->atlas, (u8)g_block_width);

	return SUCCESS;
}