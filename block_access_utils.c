#include "file_system_functions.c"
#include <stdlib.h>
#include <string.h>

// return 0 at fail (chunk is not exists)
// return 1 at success

int is_block_void(block *b)
{
    return b->id || b->data_size || b->data;
}

int get_world_layer(world *w, char letter, world_layer *out)
{
    int i = 0;
    while (w->layers[i].letter != letter && i < w->depth)
    {
        i++;
    }
    if (i == w->depth)
        return 0;
    out = &w->layers[i];
    return 1;
}

int get_block_access(world *w, char letter, int x, int y, block *ptr)
{
    world_layer *wl;
    if (get_world_layer(w, letter, wl))
        return 0;

    int chunk_x = x / wl->chunk_width;
    int chunk_y = y / wl->chunk_width;

    if (wl->chunks[chunk_x][chunk_y] == 0)
    {
        char filename[64];
        if (!chunk_load(w, letter, chunk_x, chunk_y, wl->chunks[chunk_x][chunk_y]))
            return 0;
    }

    int local_x = x % wl->chunk_width;
    int local_y = y % wl->chunk_width;

    ptr = &wl->chunks[chunk_x][chunk_y]->blocks[local_x][local_y];

    return 1;
}

int set_block(world *w, char letter, int x, int y, block *b)
{
    block *blk;
    if (get_block_access(w, letter, x, y, blk))
    {
        block_copy(blk, b);
        return 0;
    }
    return 1;
}

int clean_block(world *w, char letter, int x, int y)
{
    block *blk;
    if (get_block_access(w, letter, x, y, blk))
    {
        block_data_free(blk);
        *blk = void_block;
        return 0;
    }
    return 1;
}

int move_block_gently(world *w, char letter_from, char letter_to, int x, int y, int vx, int vy)
{
    block *source;
    block *destination;
    if (get_block_access(w, letter_from, x, y, source) && get_block_access(w, letter_to, x + vx, y + vy, destination))
    {
        if (is_block_void(destination))
        {
            block_teleport(source, destination);
            return 0;
        }
    }
    return 1;
}

int move_block_rough(world *w, char letter_from, char letter_to, int x, int y, int vx, int vy)
{
    block *source;
    block *destination;
    if (get_block_access(w, letter_from, x, y, source) && get_block_access(w, letter_to, x + vx, y + vy, destination))
    {
        block_teleport(source, destination);
        return 0;
    }
    return 1;
}