#ifndef BLOCK_OPERATIONS
#define BLOCK_OPERATIONS

#include "world_fs.h"

typedef struct
{
    layer_chunk **chunks; // array of pointers to chunks
    int x, y;             // chunk segment start position
    int w, h;             // chunk segment size
} chunk_segment;

chunk_segment chunk_segment_create(const world *w, const int index, const int chunk_x, const int chunk_y, const int width, const int height);
void chunk_segment_free(chunk_segment *cs);
block *chunk_segment_get_block_access(const chunk_segment cs, const int block_x, const int block_y);

int is_chunk_unloaded(const world_layer *wl, const int chunk_x, const int chunk_y);
block *get_block_access(const world *w, const int index, const int x, const int y);

int set_block(const world *w, const int index, const int x, const int y, const block *b);
int clean_block(const world *w, int index, const int x, const int y);
int is_move_needed(const block *destination, const block *source);

int move_block_gently(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy);
int move_block_rough(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy);
int move_block_recursive(const world *w, const int layer_index, const int x, const int y, const int vx, const int vy, const int limit_inclusive);

#endif