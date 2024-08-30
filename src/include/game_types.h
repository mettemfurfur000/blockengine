#ifndef GAME_TYPES
#define GAME_TYPES

#include "../../vec/src/vec.h"

typedef unsigned char byte;

typedef vec_t(byte) vec_byte_t;

#define SUCCESS 1
#define FAIL 0

typedef struct
{
	/*
	data format:
	fist byte - size
	data+1 to data+1+size - custom bytes for you. max 256 <3
	if the field is null, there is no data
	*/
	byte *data;

	int id;
} block;

// TOOD: limit chunk width to 180

typedef struct
{
	unsigned short new_id;
	short block_index; // points to block in chunk->blocks
} block_update;

typedef struct
{
	unsigned short data_index; // points to a data record in a snowball
	byte size;				   // size of the data
	char label;				   // label
} data_update;

typedef vec_t(block_update) vec_block_update_t;

typedef vec_t(data_update) vec_data_update_t;

// im not recomendink realocatink this strucctur...
typedef struct
{
	vec_block_update_t block_updates;
	vec_data_update_t data_updates;
	vec_byte_t updated_data_snowball;
} chunk_update;

typedef vec_t(block) vec_block_t;

typedef struct
{
	chunk_update updates;
	vec_block_t blocks;
	byte width;

	byte activity;
	// 0 - inactive, will be unloaded soon
	// 1 - active, stay loaded for a while
	// 2+ - have constantly active blocks, never unload
	// can be changed in future
} layer_chunk;

#define LAYER_FROM_WORLD(__w, __layer) (&__w->layers.data[__layer])
#define CHUNK_FROM_LAYER(__layer, layer_x, layer_y) __layer->chunks[layer_x][layer_y]
#define BLOCK_FROM_CHUNK(__chunk, x, y) __chunk->blocks.data + (y * __chunk->width) + x

#define FOREACH_BLOCK_FROM_CHUNK(__chunk, __blk, __action_block) \
	for (int y = 0; y < __chunk->width; y++)                     \
		for (int x = 0; x < __chunk->width; x++)                 \
		{                                                        \
			block *blk = BLOCK_FROM_CHUNK(__chunk, x, y);        \
			__action_block                                       \
		}

typedef struct
{
	layer_chunk ***chunks;

	int index;

	int size_x;
	int size_y;

	byte chunk_width;
} world_layer;

typedef vec_t(world_layer) vec_world_layer_t;

typedef struct
{
	vec_world_layer_t layers;

	char worldname[32];
} world;

#endif