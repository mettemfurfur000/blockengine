#include "include/block_operations.h"
#include "include/engine_events.h"

int is_chunk_unloaded(const world_layer *wl, const int chunk_x, const int chunk_y)
{
	if (!wl)
		return FAIL;

	return wl->chunks[chunk_x][chunk_y] == 0;
}

block *get_block_access(const world *w, const int index, const int x, const int y)
{
	if (!w)
		return FAIL;

	if (index >= w->layers.length)
		return FAIL;

	world_layer *wl = &w->layers.data[index];

	if (x < 0 || y < 0)
		return FAIL;

	const int chunk_x = x / wl->chunk_width;
	const int chunk_y = y / wl->chunk_width;

	if (chunk_x >= wl->size_x ||
		chunk_y >= wl->size_y)
		return FAIL;

	if (is_chunk_unloaded(wl, chunk_x, chunk_y))
		if (!chunk_load(w, index, chunk_x, chunk_y, wl->chunks[chunk_x][chunk_y]))
			return FAIL;

	const int local_x = x % wl->chunk_width;
	const int local_y = y % wl->chunk_width;

	return BLOCK_FROM_CHUNK(wl->chunks[chunk_x][chunk_y], local_x, local_y);
}

// TODO: add chunk access function so caller have more control over chunk blocks
// maybe even chunk section to have more control over region x,y by h,w (in blocks)

int set_block(const world *w, const int layer_index, const int x, const int y, const block *b)
{
	block *blk = get_block_access(w, layer_index, x, y);
	if (!blk)
		return FAIL;

	{
		block_update_event new_event = {.type = ENGINE_BLOCK_SET,
										.target_x = x,
										.target_y = y,
										.target = blk,
										.target_layer_id = layer_index,
										.previous_id = blk->id,
										.new_id = b->id};

		SDL_PushEvent((SDL_Event *)&new_event);
	}

	block_copy(blk, b);

	return SUCCESS;
}

int clean_block(const world *w, int layer_index, const int x, const int y)
{
	block *blk = get_block_access(w, layer_index, x, y);
	if (!blk)
		return FAIL;

	{
		block_update_event new_event = {.type = ENGINE_BLOCK_ERASED,
										.target_x = x,
										.target_y = y,
										.target = blk,
										.target_layer_id = layer_index,
										.previous_id = blk->id,
										.new_id = 0};

		SDL_PushEvent((SDL_Event *)&new_event);
	}

	block_erase(blk);

	return SUCCESS;
}

int is_move_needed(const block *destination, const block *source)
{
	if (!source || !destination) // no pointers == no move
		return FAIL;

	if (source == destination) // move to intself == already moved
		return FAIL;

	if (is_block_void(source) && is_block_void(destination)) // no need to move void
		return FAIL;

	return SUCCESS;
}

int move_block_gently(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy)
{
	block *source = get_block_access(w, layer_index, x, y);
	block *destination = get_block_access(w, layer_index, x + vx, y + vy);

	if (!is_move_needed(destination, source))
		return FAIL;

	if (!is_block_void(destination))
		return FAIL;

	{ // source block updated to 0
		block_update_event new_event = {.type = ENGINE_BLOCK_MOVE,
										.target_x = x,
										.target_y = y,
										.target = source,
										.target_layer_id = layer_index,
										.previous_id = source->id,
										.new_id = 0};

		SDL_PushEvent((SDL_Event *)&new_event);
	}

	{ // dest block becomes source block
		block_update_event new_event = {.type = ENGINE_BLOCK_MOVE,
										.target_x = x + vx,
										.target_y = y + vy,
										.target = destination,
										.target_layer_id = layer_index,
										.previous_id = 0,
										.new_id = destination->id};

		SDL_PushEvent((SDL_Event *)&new_event);
	}

	block_teleport(destination, source);

	return SUCCESS;
}

int move_block_rough(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy)
{
	block *source = get_block_access(w, layer_index, x, y);
	block *destination = get_block_access(w, layer_index, x + vx, y + vy);

	if (!is_move_needed(destination, source))
		return FAIL;

	{ // source block updated to 0
		block_update_event new_event = {.type = ENGINE_BLOCK_MOVE,
										.target_x = x,
										.target_y = y,
										.target = source,
										.target_layer_id = layer_index,
										.previous_id = source->id,
										.new_id = 0};

		SDL_PushEvent((SDL_Event *)&new_event);
	}

	{ // dest block was replaced with source block (Different Event id this time)
		block_update_event new_event = {.type = ENGINE_BLOCK_ERASED,
										.target_x = x + vx,
										.target_y = y + vy,
										.target = destination,
										.target_layer_id = layer_index,
										.previous_id = destination->id,
										.new_id = source->id};

		SDL_PushEvent((SDL_Event *)&new_event);
	}

	block_teleport(destination, source);

	return SUCCESS;
}

int move_block_recursive(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy, const int limit_inclusive)
{
	if (limit_inclusive == 0)
		return FAIL;

	block *source = get_block_access(w, layer_index, x, y);
	block *destination = get_block_access(w, layer_index, x + vx, y + vy);

	if (!is_move_needed(destination, source))
		return FAIL;

	if (!is_block_void(destination))
	{
		if (!move_block_recursive(w, layer_index, x + vx, y + vy,
								  vx == 0 ? 0 : vx > 0 ? 1
													   : -1,
								  vy == 0 || vx != 0 ? 0 : vy > 0 ? 1
																  : -1,
								  limit_inclusive - 1))
			return FAIL;
	}

	{ // source block updated to 0
		block_update_event new_event = {.type = ENGINE_BLOCK_MOVE,
										.target_x = x,
										.target_y = y,
										.target = source,
										.target_layer_id = layer_index,
										.previous_id = source->id,
										.new_id = 0};

		SDL_PushEvent((SDL_Event *)&new_event);
	}

	{ // dest block becomes source block
		block_update_event new_event = {.type = ENGINE_BLOCK_MOVE,
										.target_x = x + vx,
										.target_y = y + vy,
										.target = destination,
										.target_layer_id = layer_index,
										.previous_id = 0,
										.new_id = destination->id};

		SDL_PushEvent((SDL_Event *)&new_event);
	}

	block_teleport(destination, source);

	return SUCCESS;
}

#define KEEP_IN_RANGE(x, min, max) (x < min ? min : (x > max ? max : x))

chunk_segment chunk_segment_create(const world *w, const int index, int chunk_x, int chunk_y, int width, int height)
{
	chunk_segment cs = {0};

	if (!w)
		return cs;

	if (index >= w->layers.length)
		return cs;

	world_layer *wl = &w->layers.data[index];
	// fix chunk_x and chunk_y to be within the world
	chunk_x = KEEP_IN_RANGE(chunk_x, 0, wl->size_x - 1);
	chunk_y = KEEP_IN_RANGE(chunk_y, 0, wl->size_y - 1);
	// fix width and height to be within the world, and also contain at least 1 chunk
	width = KEEP_IN_RANGE(width, 1, wl->size_x - chunk_x);
	height = KEEP_IN_RANGE(height, 1, wl->size_y - chunk_y);

	cs.x = chunk_x;
	cs.y = chunk_y;

	cs.w = width;
	cs.h = height;

	cs.chunks = malloc(sizeof(void *) * (width * height));

	// copy needed chunk pointers to the chunks array, even if they are not loaded
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			cs.chunks[j * width + i] = wl->chunks[chunk_x + i][chunk_y + j];

	return cs;
}

void chunk_segment_free(chunk_segment *cs)
{
	if (!cs)
		return;
	free(cs->chunks);
}

block *chunk_segment_get_block_access(const chunk_segment cs, const int block_x, const int block_y)
{
	if (block_x < 0 || block_y < 0)
		return NULL;

	const int chunk_width = cs.chunks[0]->width;

	const int chunk_x = block_x / chunk_width;
	const int chunk_y = block_y / chunk_width;

	if (chunk_x < cs.x || chunk_y < cs.y || chunk_x >= cs.x + cs.w || chunk_y >= cs.y + cs.h) // check if chunk is in segment
		return NULL;

	const int segment_chunk_x = chunk_x - cs.x;
	const int segment_chunk_y = chunk_y - cs.y;

	layer_chunk *chunk = cs.chunks[segment_chunk_x + segment_chunk_y * cs.w];

	block *b = BLOCK_FROM_CHUNK(chunk, block_x % chunk_width, block_y % chunk_width);

	return b;
}