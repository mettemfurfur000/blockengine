#ifndef RENDERING_H
#define RENDERING_H

#include "level.h"

#define LAYER_SLICE_FLAG_FROZEN 0b00000001
#define LAYER_SLICE_FLAG_RENDER_COMPLETE 0b00000010

#define LAYER_CACHE_MODE_SLOT 0
#define LAYER_CACHE_MODE_FBO 1

// Selects how a layer is rasterized.
//   SLOT - per-block instanced slots with a sliding region cache (default).
//          Rebuilds individual tiles and keeps entity geometry per frame.
//   FBO  - whole visible region is rasterized once into an offscreen texture
//          and composited as a single quad; only entities redraw per frame.
//          Meant for fully static layers. Falls back to SLOT if the FBO is
//          incomplete.
void render_layer_set_cache_mode(layer *l, u8 mode);

typedef struct layer_slice
{
	layer *ref;
	u32 framebuffer;
	u32 framebuffer_texture;

	u32 x, y; // coordinates in the world, in pixels (most of the time 16 per block)
	u32 w, h; // width and height of visible area

	u32 old_x, old_y; // for interpolation
	u32 timestamp_old;
	u32 interp_takes;

	f32 zoom;
	u8 flags; // static layer gets rendered once in a framebuffer
} layer_slice;

typedef vec_t(layer_slice) layer_slices_t;

u8 render_layer(layer_slice slice);

#endif