#include "include/block_operations.h"

int is_chunk_unloaded(const world_layer *wl, const int chunk_x, const int chunk_y)
{
	if (!wl)
		return FAIL;

	return wl->chunks[chunk_x][chunk_y] == 0;
}

int is_two_blocks_in_the_same_chunk(const world *w, const int layer_index, const int x1, const int y1, const int x2, const int y2)
{
	world_layer *wl = &w->layers.data[layer_index];

	const int chunk_x1 = x1 / wl->chunk_width;
	const int chunk_y1 = y1 / wl->chunk_width;

	const int chunk_x2 = x2 / wl->chunk_width;
	const int chunk_y2 = y2 / wl->chunk_width;

	if (chunk_x1 != chunk_x2 || chunk_y1 != chunk_y2)
		return FAIL;

	return SUCCESS;
}

layer_chunk *get_chunk_access(const world *w, const int index, const int x, const int y)
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

	return CHUNK_FROM_LAYER(wl, chunk_x, chunk_y);
}

int get_access_context(access_context *target, const world *__w, const int index, const int x, const int y, const int w, const int h)
{
	world_layer *wl = &__w->layers.data[index];

	if (!target)
		return FAIL;

	int chunk_x = x / wl->chunk_width;
	int chunk_y = y / wl->chunk_width;
	int chunk_w = (x + w) / wl->chunk_width + 1;
	int chunk_h = (y + h) / wl->chunk_width + 1;

	chunk_x = max(0, chunk_x);
	chunk_y = max(0, chunk_y);

	chunk_w = min(chunk_w, wl->size_x);
	chunk_h = min(chunk_h, wl->size_y);

	int total_needed_chunks = chunk_w * chunk_h;

	if (target->chunks && (target->h * target->w) < total_needed_chunks)
		free(target->chunks);

	target->chunks = malloc(sizeof(layer_chunk *) * total_needed_chunks);

	// copy the tiny part of the world into the access_context struct
	for (int j = 0; j < chunk_h; j++)
		for (int i = 0; i < chunk_w; i++)
		{
			layer_chunk *c = get_chunk_access(__w, index, chunk_x + i, chunk_y + j);
			target->chunks[j * chunk_w + i] = c ? c : 0;
		}

	target->x = chunk_x;
	target->y = chunk_y;
	target->w = chunk_w;
	target->h = chunk_h;

	target->chunk_width = wl->chunk_width;

	return SUCCESS;
}

block *get_block_access_context(const access_context *c, const int x, const int y)
{
	if (!c)
		return NULL;

	if (x < 0 || y < 0)
		return NULL;

	if (x >= c->w * c->chunk_width || y >= c->h * c->chunk_width)
		return NULL;

	const int local_x = x % c->chunk_width;
	const int local_y = y % c->chunk_width;

	const int chunk_x = x / c->chunk_width; // those points on a chunk in layer structure
	const int chunk_y = y / c->chunk_width;

	// convert them into access_context chunk array index
	int index = (chunk_y - c->y) * c->w + (chunk_x - c->x);

	if (index > c->w * c->h)
		return NULL;

	return BLOCK_FROM_CHUNK(c->chunks[index], local_x, local_y);
}

block *get_block_access(const layer_chunk *c, const int x, const int y)
{
	const int local_x = x % c->width;
	const int local_y = y % c->width;

	return BLOCK_FROM_CHUNK(c, local_x, local_y);
}

int set_block(const world *w, const int index, const int x, const int y, const block *b)
{
	layer_chunk *c = get_chunk_access(w, index, x, y);
	block *blk = get_block_access(c, x, y);
	if (!blk)
		return FAIL;

	block_copy(blk, b);
	record_block_update(&c->updates, b->id, blk - c->blocks.data); // TODO: maybe use a better way to get the index
	return SUCCESS;
}

int clean_block(const world *w, int index, const int x, const int y)
{
	layer_chunk *c = get_chunk_access(w, index, x, y);
	block *blk = get_block_access(c, x, y);
	if (!blk)
		return FAIL;

	block_data_free(blk);
	make_void_block(blk);
	record_block_update(&c->updates, 0, blk - c->blocks.data);

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
	const int dest_x = x + vx;
	const int dest_y = x + vy;

	layer_chunk *src_c = get_chunk_access(w, layer_index, x, y);
	layer_chunk *dest_c = is_two_blocks_in_the_same_chunk(w, layer_index, x, y, dest_x, dest_y)
							  ? src_c
							  : get_chunk_access(w, layer_index, dest_x, dest_y);
	block *source = get_block_access(src_c, x, y);
	block *destination = get_block_access(dest_c, dest_x, dest_y);

	if (!is_move_needed(destination, source))
		return FAIL;

	if (!is_block_void(destination))
		return FAIL;

	block_teleport(destination, source);

	record_block_update(&src_c->updates, source->id, source - src_c->blocks.data);
	record_block_update(&dest_c->updates, destination->id, destination - dest_c->blocks.data);

	return SUCCESS;
}

int move_block_rough(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy)
{
	const int dest_x = x + vx;
	const int dest_y = x + vy;

	layer_chunk *src_c = get_chunk_access(w, layer_index, x, y);
	layer_chunk *dest_c = is_two_blocks_in_the_same_chunk(w, layer_index, x, y, dest_x, dest_y)
							  ? src_c
							  : get_chunk_access(w, layer_index, dest_x, dest_y);
	block *source = get_block_access(src_c, x, y);
	block *destination = get_block_access(dest_c, dest_x, dest_y);

	if (!is_move_needed(destination, source))
		return FAIL;

	block_teleport(destination, source);

	record_block_update(&src_c->updates, source->id, source - src_c->blocks.data);
	record_block_update(&dest_c->updates, destination->id, destination - dest_c->blocks.data);

	return SUCCESS;
}

int move_block_recursive(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy, const int limit_inclusive)
{
	if (limit_inclusive == 0)
		return FAIL;

	const int dest_x = x + vx;
	const int dest_y = x + vy;

	layer_chunk *src_c = get_chunk_access(w, layer_index, x, y);
	layer_chunk *dest_c = is_two_blocks_in_the_same_chunk(w, layer_index, x, y, dest_x, dest_y)
							  ? src_c
							  : get_chunk_access(w, layer_index, dest_x, dest_y);
	block *source = get_block_access(src_c, x, y);
	block *destination = get_block_access(dest_c, dest_x, dest_y);

	if (!is_move_needed(destination, source))
		return FAIL;

	if (!is_block_void(destination))
	{
		if (!move_block_recursive(w, layer_index, x + vx, dest_y,
								  vx == 0 ? 0 : vx > 0 ? 1
													   : -1,
								  vy == 0 || vx != 0 ? 0 : vy > 0 ? 1
																  : -1,
								  limit_inclusive - 1))
			return FAIL;
	}

	block_teleport(destination, source);

	record_block_update(&src_c->updates, source->id, source - src_c->blocks.data);
	record_block_update(&dest_c->updates, destination->id, destination - dest_c->blocks.data);

	return SUCCESS;
}
