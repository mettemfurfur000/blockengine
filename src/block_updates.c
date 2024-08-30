#include "include/block_updates.h"

void record_block_update(chunk_update *packet, int id, int index)
{
    const block_update new_update = {id, index};
    (void)vec_push(&packet->block_updates, new_update);
}

void record_data_update(chunk_update *packet, block *b, char letter)
{
    byte *data = data_get_ptr(b->data, letter);
    byte size = (-1)[data];

    int prev_vec_size = packet->updated_data_snowball.length;

    vec_pusharr(&packet->updated_data_snowball, data, size);

    const data_update new_update = {.data_index = prev_vec_size, .label = letter, .size = size};

    (void)vec_push(&packet->data_updates, new_update);
}

void record_data_delete(chunk_update *packet, block *b, char letter)
{
    const data_update new_update = {.data_index = 0, .label = letter, .size = 0};

    (void)vec_push(&packet->data_updates, new_update);
}
