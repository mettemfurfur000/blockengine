#include "game_types.h"
#include <stdlib.h>
#include <string.h>

void block_data_free(block *b)
{
    if (!b->data)
        return;
    free(b->data);
    b->data = 0;
    b->data_size = 0;
}

void block_data_alloc(block *b, int size)
{
    if (b->data)
        free(b->data);

    b->data = (byte *)calloc(size, 1);
    b->data_size = size;
}

void block_data_resize(block *b, int change)
{
    int new_size = b->data_size + change;
    byte *buffer = (byte *)calloc(new_size, 1);

    memcpy(buffer, b->data, new_size);
    free(b->data);

    b->data_size = new_size;
    b->data = buffer;
}

void block_erase(block *b)
{
    block_data_free(b);
    *b = void_block;
}

void block_copy(block *dest, block *src)
{
    block_data_free(dest);

    memcpy(dest, src, sizeof(block));
    memcpy(dest->data, src->data, src->data_size);
}

void block_teleport(block *dest, block *src)
{
    block_copy(dest,src);

    block_erase(src);
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

void chunk_alloc(layer_chunk *l, int width)
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
        }
        free(wl->chunks[i]);
    }

    free(wl->chunks);

    wl->chunks = 0;
    wl->size_x = 0;
    wl->size_y = 0;
    wl->chunk_width = 0;
}

void world_layer_alloc(world_layer *wl, int size_x, int size_y, int chunk_width)
{
    world_layer_free(wl);

    wl->chunks = (layer_chunk ***)calloc(size_x, sizeof(layer_chunk **));
    for (int i = 0; i < size_x; i++)
    {
        wl->chunks[i] = (layer_chunk **)calloc(size_y, sizeof(layer_chunk *));
        for (int j = 0; j < size_y; j++)
        {
            chunk_alloc(wl->chunks[i][j],chunk_width);
        }
    }

    wl->size_x = size_x;
    wl->size_y = size_y;
    wl->chunk_width = chunk_width;
}
