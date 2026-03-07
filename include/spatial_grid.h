#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H 1

#include "general.h"

#define AUTOTILE_CACHE_INVALID 0xFF

typedef struct
{
    u16 x;
    u16 y;
    u8 cached_autotile_frame;
} block_pos;

typedef struct spatial_cell
{
    u32 count;
    u32 capacity;
    block_pos *positions;
} spatial_cell;

typedef struct spatial_grid
{
    u16 cell_size;
    u16 grid_width;
    u16 grid_height;
    spatial_cell *cells;
} spatial_grid;

spatial_grid spatial_grid_create(u16 layer_width, u16 layer_height, u16 cell_size);
void spatial_grid_destroy(spatial_grid *grid);
void spatial_grid_update(spatial_grid *grid, u16 x, u16 y, u64 old_id, u64 new_id);
void spatial_grid_build_from_layer(spatial_grid *grid, u16 layer_width, u16 layer_height, u8 block_size, u8 *blocks, u64 total_bytes_per_block);
void spatial_grid_get_visible(spatial_grid *grid, i32 start_x, i32 start_y, i32 end_x, i32 end_y,
                              void *callback_ctx,
                              void (*callback)(void *ctx, u16 x, u16 y, u8 *cached_frame));

void spatial_grid_set_cached_frame(spatial_grid *grid, u16 x, u16 y, u8 frame);

#endif
