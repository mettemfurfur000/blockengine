#ifndef BLOCK_UPDATES_H
#define BLOCK_UPDATES_H

#include "game_types.h"
#include "block_operations.h"
#include "data_manipulations.h"

// this is used to record regular block updates in a chunk
void record_block_update(chunk_update *packet, int id, int index);

// this is used to record data updates in a block of some chunk
void record_data_update(chunk_update *packet, block *b, char letter);
void record_data_delete(chunk_update *packet, block *b, char letter);

#endif
