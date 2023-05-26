#include "file_system_functions.c"
#include <stdlib.h>
#include <string.h>

// return 0 at fail (chunk is not exists)
// return 1 at success

int is_block_void(block *b)
{
    return b->id || b->data_size || b->data || b->custom_behaviour_function;
}

int get_block_access(world_layer *w, int x, int y, block *ptr)
{
    int chunk_x = x / w->chunk_width;
    int chunk_y = y / w->chunk_width;

    if (w->chunks[chunk_x][chunk_y] == 0)
    {
        char[64] filename;
        make_chunk_name(filename, w->letter, x, y) if (!load_chunk(w->chunks[chunk_x][chunk_y], filename)) return 1;
    }

    int local_x = x % w->chunk_width;
    int local_y = y % w->chunk_width;

    ptr = &w->chunks[chunk_x][chunk_y]->blocks[local_x][local_y];

    return 0;
}

int set_block(world_layer *w, int x, int y, block *b)
{
    block *blk;
    if (get_block_access(w, x, y, blk))
    {
        block_copy(blk, b);
        return 1;
    }
    return 0;
}

int clean_block(world_layer *w, int x, int y)
{
    block *blk;
    if (get_block_access(w, x, y, blk))
    {
        block_data_free(blk);
        *blk = void_block;
        return 1;
    }
    return 0;
}

int move_block_gently(world_layer *w, int x, int y, int vx, int vy)
{
    block *source;
    block *destination;
    if (get_block_access(w, x, y, source) && get_block_access(w, x + vx, y + vy, destination))
    {
        if (is_block_void(destination))
        {
            block_teleport(source, destination);
            return 1;
        }
    }
    return 0;
}

int move_block_rough(world_layer *w, int x, int y, int vx, int vy)
{
    block *source;
    block *destination;
    if (get_block_access(w, x, y, source) && get_block_access(w, x + vx, y + vy, destination))
    {
        block_teleport(source, destination);
        return 1;
    }
    return 0;
}