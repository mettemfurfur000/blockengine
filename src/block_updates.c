#include "include/block_updates.h"

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

	if (index >= w->depth)
		return FAIL;

	world_layer *wl = &w->layers[index];

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

	return &wl->chunks[chunk_x][chunk_y]->blocks[local_x][local_y];
}

int set_block(const world *w, const int index, const int x, const int y, const block *b)
{
	block *blk = get_block_access(w, index, x, y);
	if (!blk)
		return FAIL;

	block_copy(blk, b);
	return SUCCESS;
}

int clean_block(const world *w, int index, const int x, const int y)
{
	block *blk = get_block_access(w, index, x, y);
	if (!blk)
		return FAIL;

	block_data_free(blk);
	make_void_block(blk);

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

	block_teleport(destination, source);

	return SUCCESS;
}

int move_block_rough(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy)
{
	block *source = get_block_access(w, layer_index, x, y);
	block *destination = get_block_access(w, layer_index, x + vx, y + vy);

	if (!is_move_needed(destination, source))
		return FAIL;

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

	block_teleport(destination, source);

	return SUCCESS;
}
