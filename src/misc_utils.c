#ifndef MISC_UTILS_H
#define MISC_UTILS_H
#include "memory_control_functions.c"

void block_set_random(block *b)
{
    block_data_free(b);

    b->id = rand();
    b->data_size = rand() % 32;
    block_data_alloc(b, b->data_size);
    for (size_t i = 0; i < b->data_size; i++)
    {
        b->data[i] = 'A' + rand() % 52;
    }
}

void chunk_random_fill(layer_chunk *l)
{
    for (int i = 0; i < l->width; i++)
    {
        for (int j = 0; j < l->width; j++)
        {
            block_set_random(&l->blocks[i][j]);
        }
    }
}

#endif