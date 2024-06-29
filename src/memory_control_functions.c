#include "include/memory_control_functions.h"

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
	for (int i = 0; i < a->width; i++)
	{
		for (int j = 0; j < a->width; j++)
		{
			status &= is_block_equal(&a->blocks[i][j], &b->blocks[i][j]);
		}
	}
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

void chunk_free(layer_chunk *l)
{
	if (!l->blocks)
		return;

	for (int i = 0; i < l->width; i++)
	{
		for (int j = 0; j < l->width; j++)
		{
			block_erase(&l->blocks[i][j]);
		}
		free(l->blocks[i]);
	}
	free(l->blocks);

	l->blocks = 0;
	l->width = 0;
}

void chunk_alloc(layer_chunk *l, const int width)
{
	chunk_free(l);

	l->blocks = (block **)calloc(width, sizeof(block *));

	for (int i = 0; i < width; i++)
	{
		l->blocks[i] = (block *)calloc(width, sizeof(block));
	}

	l->width = width;
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

	w->depth = depth;
	w->layers = (world_layer *)calloc(depth, sizeof(world_layer));
	return w;
}

void world_free(world *w)
{
	free(w->layers);
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

	for (int i = 0; i < l->width; i++)
		for (int j = 0; j < l->width; j++)
			block_set_random(&l->blocks[i][j]);
}
