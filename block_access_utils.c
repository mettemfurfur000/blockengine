#include "file_system_functions.c"
#include <stdlib.h>
#include <string.h>

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
        return FAIL;
    out = &w->layers[i];
    return SUCCESS;
}

int get_block_access(world *w, char letter, int x, int y, block *ptr)
{
    world_layer *wl;
    if (!get_world_layer(w, letter, wl))
        return FAIL;

    int chunk_x = x / wl->chunk_width;
    int chunk_y = y / wl->chunk_width;

    if (wl->chunks[chunk_x][chunk_y] == 0)
    {
        if (!chunk_load(w, letter, chunk_x, chunk_y, wl->chunks[chunk_x][chunk_y]))
            return FAIL;
    }

    int local_x = x % wl->chunk_width;
    int local_y = y % wl->chunk_width;

    ptr = &wl->chunks[chunk_x][chunk_y]->blocks[local_x][local_y];

    return SUCCESS;
}

int set_block(world *w, char letter, int x, int y, block *b)
{
    block *blk;
    if (!get_block_access(w, letter, x, y, blk))
        return FAIL;
    
    block_copy(blk, b);
    return SUCCESS;
}

int clean_block(world *w, char letter, int x, int y)
{
    block *blk;
    if (!get_block_access(w, letter, x, y, blk))
        return FAIL;

    block_data_free(blk);
    *blk = void_block;

    return SUCCESS;
}

int move_block_gently(world *w, char letter_from, char letter_to, int x, int y, int vx, int vy)
{
    block *source;
    block *destination;

    if (get_block_access(w, letter_from, x, y, source) && get_block_access(w, letter_to, x + vx, y + vy, destination))
        return FAIL;

    if (is_block_void(destination))
        return FAIL;

    block_teleport(source, destination);

    return SUCCESS;
}

int move_block_rough(world *w, char letter_from, char letter_to, int x, int y, int vx, int vy)
{
    block *source;
    block *destination;

    if (!get_block_access(w, letter_from, x, y, source) && get_block_access(w, letter_to, x + vx, y + vy, destination))
        return FAIL;

    block_teleport(source, destination);

    return SUCCESS;
}

int move_block_recursive(world *w, char letter_from, char letter_to, int x, int y, int vx, int vy, int limit_inclusive = 2)
{
    if (limit_inclusive == 0)
        return FAIL;

    block *source;
    block *destination;

    if (!(get_block_access(w, letter_from, x, y, source) && get_block_access(w, letter_to, x + vx, y + vy, destination)))
        return FAIL;

    if (!is_block_void(destination))
        if (!move_block_recursive(w, letter_to, letter_to, x + vx, y + vy, vx > 0 ? 1 : -1, vy > 0 ? 1 : -1, limit_inclusive - 1))
            return FAIL;

    block_teleport(source, destination);

    return SUCCESS;
}