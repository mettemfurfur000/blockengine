#include "include/block_memory_control.h"

void make_void_block(block *b)
{
	b->id = 0;
	b->data = 0;
}

int is_data_equal(const block *a, const block *b)
{
	if (!a->data && !b->data)
		return SUCCESS; // both blocks have no data, so data is equal...
	if (!a->data || !b->data)
		return FAIL;
	if (a->data[0] != b->data[0])
		return FAIL;

	for (int i = 1, size = a->data[0] + 1; i < size; i++)
		if (a->data[i] != b->data[i])
			return FAIL;

	return SUCCESS;
}

int is_block_void(const block *b)
{
	return !(b->id || b->data);
}

int is_block_equal(const block *a, const block *b)
{
	return a->id == b->id && is_data_equal(a, b);
}

int is_chunk_equal(const layer_chunk *a, const layer_chunk *b)
{
	if (a->width != b->width)
		return FAIL;

	int status = 1;
	FOREACH_BLOCK_FROM_CHUNK(a, blk, {
		status &= is_block_equal(blk, BLOCK_FROM_CHUNK(b, x, y));
	})

	return status;
}

void block_data_free(block *b)
{
	if (!b->data)
		return;
	free(b->data);
	b->data = 0;
}

void block_data_alloc(block *b, int size)
{
	if (b->data)
		free(b->data);
	if (!size)
	{
		b->data = 0;
		return;
	}

	b->data = (byte *)calloc(size + 1, 1);
	b->data[0] = (byte)size;
}

void block_data_resize(block *b, int change)
{
	int new_size = b->data[0] + change;
	byte *buffer = (byte *)calloc(new_size + 1, 1);

	memcpy(buffer + 1, b->data + 1, new_size);
	free(b->data);

	b->data = buffer;
	b->data[0] = new_size;
}

void block_erase(block *b)
{
	block_data_free(b);
	make_void_block(b);
}

void block_copy(block *dest, const block *src)
{
	block_data_free(dest);
	// copy fields
	memcpy(dest, src, sizeof(block));
	// copy data itself
	dest->data = 0;
	if (!src->data)
		return;

	block_data_alloc(dest, src->data[0]);
	memcpy(dest->data + 1, src->data + 1, src->data[0]);
}

void block_init(block *b, const int id, const int data_size, const char *data)
{
	b->id = id;
	block_data_alloc(b, data_size);
	if (!data_size)
		return;
	memcpy(b->data + 1, data + 1, b->data[0]);
}

void block_teleport(block *dest, block *src)
{
	block_copy(dest, src);

	block_erase(src);
}

void block_swap(block *a, block *b)
{
	block tmp;

	memcpy(&tmp, a, sizeof(block)); // why not block_copy? we dont want to realloc things, just swap all fields and thats it

	memcpy(a, b, sizeof(block));

	memcpy(b, &tmp, sizeof(block));
}

void chunk_free(layer_chunk *chnk)
{
	if (chnk->blocks.length == 0)
		return;

	FOREACH_BLOCK_FROM_CHUNK(chnk, blk, {
		block_erase(blk);
	})

	vec_deinit(&chnk->blocks);

	chnk->width = 0;
}

void chunk_alloc(layer_chunk *l, const int width)
{
	chunk_free(l);

	vec_init(&l->blocks);
	vec_reserve(&l->blocks, width * width);
	l->blocks.length = width * width;
	l->width = width;
	memset(l->blocks.data, 0, l->blocks.length * sizeof(block));
}

void world_layer_free(world_layer *wl)
{
	if (!wl->chunks)
		return;

	for (int i = 0; i < wl->size_x; i++)
	{
		for (int j = 0; j < wl->size_y; j++)
		{
			chunk_free(wl->chunks[i][j]);
			free(wl->chunks[i][j]);
		}
		free(wl->chunks[i]);
	}

	free(wl->chunks);

	wl->chunks = 0;
	wl->size_x = 0;
	wl->size_y = 0;
	wl->chunk_width = 0;
}

void world_layer_alloc(world_layer *wl, const int size_x, const int size_y, const int chunk_width, const int index)
{
	world_layer_free(wl);

	wl->chunks = (layer_chunk ***)calloc(size_x, sizeof(layer_chunk **));
	for (int i = 0; i < size_x; i++)
	{
		wl->chunks[i] = (layer_chunk **)calloc(size_y, sizeof(layer_chunk *));
		for (int j = 0; j < size_y; j++)
		{
			wl->chunks[i][j] = (layer_chunk *)calloc(1, sizeof(layer_chunk));
			chunk_alloc(wl->chunks[i][j], chunk_width);
		}
	}

	wl->size_x = size_x;
	wl->size_y = size_y;
	wl->chunk_width = chunk_width;
	wl->index = index;
}

world *world_make(const int depth, const char *name_to_copy_from)
{
	world *w = (world *)calloc(1, sizeof(world));

	strcpy(w->worldname, name_to_copy_from);

	vec_init(&w->layers);
	vec_reserve(&w->layers, depth);
	for (int i = 0; i < depth; i++)
		memset(&w->layers.data[i], 0, sizeof(world_layer));

	w->layers.length = depth;

	return w;
}

void world_free(world *w)
{
	vec_deinit(&w->layers);
	free(w);
}

// some useful functions

void block_set_random(block *b)
{
	block_data_free(b);

	b->id = rand();
	// generate data at 0.5% situations
	if (rand() % 1000 > 5)
	{
		b->data = 0;
		return;
	}

	int size = 1 + rand() % 255;
	block_data_alloc(b, size);

	for (size_t i = 1; i < size + 1; i++)
	{
		b->data[i] = 'A' + rand() % 52;
	}
}

void chunk_random_fill(layer_chunk *l, const unsigned int seed)
{
	if (seed)
		srand(seed);

	FOREACH_BLOCK_FROM_CHUNK(l, blk, {
		block_set_random(blk);
	})
}
