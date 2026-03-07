#include "include/spatial_grid.h"
#include "include/logging.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CELL_CAPACITY 16

spatial_grid spatial_grid_create(u16 layer_width, u16 layer_height, u16 cell_size)
{
    spatial_grid grid = {0};

    grid.cell_size = cell_size;
    grid.grid_width = (layer_width + cell_size - 1) / cell_size;
    grid.grid_height = (layer_height + cell_size - 1) / cell_size;

    grid.cells = (spatial_cell *)calloc((u32)grid.grid_width * grid.grid_height, sizeof(spatial_cell));
    if (!grid.cells)
    {
        LOG_ERROR("Failed to allocate spatial grid cells");
        return grid;
    }

    for (u32 i = 0; i < (u32)grid.grid_width * grid.grid_height; i++)
    {
        grid.cells[i].capacity = INITIAL_CELL_CAPACITY;
        grid.cells[i].positions = (block_pos *)malloc(INITIAL_CELL_CAPACITY * sizeof(block_pos));
        if (!grid.cells[i].positions)
        {
            LOG_ERROR("Failed to allocate spatial grid cell storage");
            spatial_grid_destroy(&grid);
            return grid;
        }
    }

    return grid;
}

void spatial_grid_destroy(spatial_grid *grid)
{
    if (!grid || !grid->cells)
        return;

    for (u32 i = 0; i < (u32)grid->grid_width * grid->grid_height; i++)
    {
        SAFE_FREE(grid->cells[i].positions);
    }
    free(grid->cells);
    grid->cells = NULL;
}

void spatial_grid_build_from_layer(spatial_grid *grid, u16 layer_width, u16 layer_height, u8 block_size, u8 *blocks, u64 total_bytes_per_block)
{
    if (!grid || !blocks)
        return;

    for (u32 y = 0; y < layer_height; y++)
    {
        for (u32 x = 0; x < layer_width; x++)
        {
            u64 id = 0;
            u8 *ptr = blocks + (y * layer_width + x) * total_bytes_per_block;

            switch (block_size)
            {
            case 1:
                id = *(u8 *)ptr;
                break;
            case 2:
                id = *(u16 *)ptr;
                break;
            case 4:
                id = *(u32 *)ptr;
                break;
            case 8:
                id = *(u64 *)ptr;
                break;
            default:
                continue;
            }

            if (id != 0)
            {
                spatial_grid_update(grid, (u16)x, (u16)y, 0, id);
            }
        }
    }
}

static u32 spatial_grid_get_cell_index(spatial_grid *grid, u16 x, u16 y)
{
    u16 cell_x = x / grid->cell_size;
    u16 cell_y = y / grid->cell_size;
    return (u32)cell_y * grid->grid_width + cell_x;
}

static void spatial_grid_cell_add(spatial_cell *cell, u16 x, u16 y)
{
    if (cell->count >= cell->capacity)
    {
        cell->capacity *= 2;
        block_pos *new_positions = (block_pos *)realloc(cell->positions, cell->capacity * sizeof(block_pos));
        if (!new_positions)
        {
            LOG_ERROR("Failed to realloc spatial cell");
            return;
        }
        cell->positions = new_positions;
    }
    cell->positions[cell->count].x = x;
    cell->positions[cell->count].y = y;
    cell->count++;
}

static void spatial_grid_cell_remove(spatial_cell *cell, u16 x, u16 y)
{
    for (u32 i = 0; i < cell->count; i++)
    {
        if (cell->positions[i].x == x && cell->positions[i].y == y)
        {
            cell->count--;
            if (i < cell->count)
            {
                memmove(&cell->positions[i], &cell->positions[i + 1],
                        (cell->count - i) * sizeof(block_pos));
            }
            return;
        }
    }
}

void spatial_grid_update(spatial_grid *grid, u16 x, u16 y, u64 old_id, u64 new_id)
{
    if (!grid || !grid->cells)
        return;
    if (x >= grid->grid_width * grid->cell_size || y >= grid->grid_height * grid->cell_size)
        return;

    u32 cell_index = spatial_grid_get_cell_index(grid, x, y);
    spatial_cell *cell = &grid->cells[cell_index];

    if (old_id != 0 && old_id != new_id)
    {
        spatial_grid_cell_remove(cell, x, y);
    }

    if (new_id != 0)
    {
        spatial_grid_cell_add(cell, x, y);
    }
}

void spatial_grid_get_visible(spatial_grid *grid, i32 start_x, i32 start_y, i32 end_x, i32 end_y,
                              void *callback_ctx,
                              void (*callback)(void *ctx, u16 x, u16 y))
{
    if (!grid || !grid->cells)
        return;

    i32 cell_start_x = start_x < 0 ? 0 : start_x;
    i32 cell_start_y = start_y < 0 ? 0 : start_y;
    i32 cell_end_x = end_x > (i32)(grid->grid_width * grid->cell_size) ? (i32)(grid->grid_width * grid->cell_size) : end_x;
    i32 cell_end_y = end_y > (i32)(grid->grid_height * grid->cell_size) ? (i32)(grid->grid_height * grid->cell_size) : end_y;

    i32 cell_x_start = cell_start_x / (i32)grid->cell_size;
    i32 cell_y_start = cell_start_y / (i32)grid->cell_size;
    i32 cell_x_end = (cell_end_x + grid->cell_size - 1) / (i32)grid->cell_size;
    i32 cell_y_end = (cell_end_y + grid->cell_size - 1) / (i32)grid->cell_size;

    for (i32 cy = cell_y_start; cy < cell_y_end; cy++)
    {
        for (i32 cx = cell_x_start; cx < cell_x_end; cx++)
        {
            if (cx < 0 || cx >= (i32)grid->grid_width || cy < 0 || cy >= (i32)grid->grid_height)
                continue;

            u32 cell_index = (u32)cy * grid->grid_width + (u32)cx;
            spatial_cell *cell = &grid->cells[cell_index];

            for (u32 i = 0; i < cell->count; i++)
            {
                u16 bx = cell->positions[i].x;
                u16 by = cell->positions[i].y;

                if ((i32)bx >= cell_start_x && (i32)bx < cell_end_x &&
                    (i32)by >= cell_start_y && (i32)by < cell_end_y)
                {
                    callback(callback_ctx, bx, by);
                }
            }
        }
    }
}
