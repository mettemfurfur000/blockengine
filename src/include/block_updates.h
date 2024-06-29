#ifndef BLOCK_UPDATES
#define BLOCK_UPDATES

#include "file_system_functions.h"

int is_chunk_unloaded(const world_layer *wl, const int chunk_x, const int chunk_y);
block *get_block_access(const world *w, const int index, const int x, const int y);

int set_block(const world *w, const int index, const int x, const int y, const block *b);
int clean_block(const world *w, int index, const int x, const int y);
int is_move_needed(const block *destination, const block *source);

int move_block_gently(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy);
int move_block_rough(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy);
int move_block_recursive(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy, const int limit_inclusive);

#endif