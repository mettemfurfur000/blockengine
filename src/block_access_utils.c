#ifndef BLOCK_ACCESS_UTILS_H
#define BLOCK_ACCESS_UTILS_H 1

#include "file_system_functions.c"

int get_block_access(world *w, int index, int x, int y, block *ptr)
{
	world_layer *wl;

	if (index >= w->depth)
		return FAIL;

	wl = &w->layers[index];

	int chunk_x = x / wl->chunk_width;
	int chunk_y = y / wl->chunk_width;

	if (wl->chunks[chunk_x][chunk_y] == 0)
	{
		if (!chunk_load(w, index, chunk_x, chunk_y, wl->chunks[chunk_x][chunk_y]))
			return FAIL;
	}

	int local_x = x % wl->chunk_width;
	int local_y = y % wl->chunk_width;

	ptr = &wl->chunks[chunk_x][chunk_y]->blocks[local_x][local_y];

	return SUCCESS;
}

int set_block(world *w, int index, int x, int y, block *b)
{
	block *blk;
	if (!get_block_access(w, index, x, y, blk))
		return FAIL;

	block_copy(blk, b);
	return SUCCESS;
}

int clean_block(world *w, int index, int x, int y)
{
	block *blk;
	if (!get_block_access(w, index, x, y, blk))
		return FAIL;

	block_data_free(blk);
	*blk = void_block;

	return SUCCESS;
}

int move_block_gently(world *w, int index_from, int index_to, int x, int y, int vx, int vy)
{
	block *source;
	block *destination;

	if (get_block_access(w, index_from, x, y, source) && get_block_access(w, index_to, x + vx, y + vy, destination))
		return FAIL;

	if (is_block_void(destination))
		return FAIL;

	block_teleport(source, destination);

	return SUCCESS;
}

int move_block_rough(world *w, int index_from, int index_to, int x, int y, int vx, int vy)
{
	block *source;
	block *destination;

	if (!get_block_access(w, index_from, x, y, source) && get_block_access(w, index_to, x + vx, y + vy, destination))
		return FAIL;

	block_teleport(source, destination);

	return SUCCESS;
}

int move_block_recursive(world *w, int index_from, int index_to, int x, int y, int vx, int vy, int limit_inclusive)
{
	if (limit_inclusive == 0)
		return FAIL;

	block *source;
	block *destination;

	if (!(get_block_access(w, index_from, x, y, source) && get_block_access(w, index_to, x + vx, y + vy, destination)))
		return FAIL;

	if (!is_block_void(destination))
		if (!move_block_recursive(w, index_to, index_to, x + vx, y + vy, vx > 0 ? 1 : -1, vy > 0 ? 1 : -1, limit_inclusive - 1))
			return FAIL;

	block_teleport(source, destination);

	return SUCCESS;
}

#endif