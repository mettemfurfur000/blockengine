#ifndef BLOCK_OPERATIONS
#define BLOCK_OPERATIONS

#include "world_fs.h"
#include "block_updates.h"

typedef struct
{ // special struct with array of chunks for easier access to blocks
    layer_chunk **chunks;
    int x, y, w, h;
    int chunk_width;
} access_context;

int is_chunk_unloaded(const world_layer *wl, const int chunk_x, const int chunk_y);
layer_chunk *get_chunk_access(const world *w, const int index, const int x, const int y);

int get_access_context(access_context *target, const world *__w, const int layer_index, const int x, const int y, const int w, const int h);
block *get_block_access_context(const access_context *c, const int x, const int y);

block *get_block_access(const layer_chunk *c, const int x, const int y);

int is_two_blocks_in_the_same_chunk(const world *w, const int layer_index, const int x1, const int y1, const int x2, const int y2);

int set_block(const world *w, const int index, const int x, const int y, const block *b);
int clean_block(const world *w, int index, const int x, const int y);
int is_move_needed(const block *destination, const block *source);

int move_block_gently(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy);
int move_block_rough(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy);
int move_block_recursive(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy, const int limit_inclusive);

#endif