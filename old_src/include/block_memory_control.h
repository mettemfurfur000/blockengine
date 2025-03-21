#ifndef BLOCK_MEMORY_CONTROL
#define BLOCK_MEMORY_CONTROL

#include <stdlib.h>
#include <string.h>

#include "engine_types.h"
#include "engine_events.h"

void make_void_block(block *b);

int is_data_equal(const block *a, const block *b);
int is_block_void(const block *b);
int is_block_equal(const block *a, const block *b);
int is_chunk_equal(const layer_chunk *a, const layer_chunk *b);

void block_data_free(block *b);
void block_data_alloc(block *b, int size);

void block_data_resize(block *b, int change);

void block_erase(block *b);
void block_copy(block *dest, const block *src);
void block_init(block *b, const int id, const int data_size, const char *data);
void block_teleport(block *dest, block *src);
void block_swap(block *a, block *b);

void chunk_free(layer_chunk *l);
void chunk_alloc(layer_chunk *l, const int width);

void world_layer_free(world_layer *wl);
void world_layer_alloc(world_layer *wl, const int size_x, const int size_y, const int chunk_width, const int index);
world *world_make(const int depth, const char *name_to_copy_from);
void world_free(world *w);

// some useful functions

void block_set_random(block *b);
void chunk_random_fill(layer_chunk *l, const unsigned int seed);

#endif