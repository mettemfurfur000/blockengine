#include "include/multiblock_entity.h"
#include "include/logging.h"

#include <stdlib.h>
#include <string.h>

multiblock_shape *multiblock_shape_create(u16 width, u16 height, const u64 *grid)
{
    if (width == 0 || height == 0)
        return NULL;

    multiblock_shape *s = (multiblock_shape *)calloc(1, sizeof(multiblock_shape));
    if (!s)
        return NULL;

    s->width = width;
    s->height = height;
    size_t n = (size_t)width * (size_t)height;

    s->grid = (u64 *)calloc(n, sizeof(u64));
    if (!s->grid)
    {
        free(s);
        return NULL;
    }

    if (grid)
        memcpy(s->grid, grid, n * sizeof(u64));

    s->props = NULL;
    s->props_count = 0;

    return s;
}

void multiblock_shape_destroy(multiblock_shape *s)
{
    if (!s)
        return;
    SAFE_FREE(s->grid);
    SAFE_FREE(s->props);
    free(s);
}

u64 multiblock_shape_get(const multiblock_shape *s, u16 x, u16 y)
{
    if (!s)
        return 0;
    if (x >= s->width || y >= s->height)
        return 0;
    size_t idx = (size_t)y * s->width + x;
    return s->grid[idx];
}

unsigned int multiblock_shape_iterate_nonzero(const multiblock_shape *s, void (*cb)(u16 x, u16 y, u64 id, void *user), void *user)
{
    if (!s || !cb)
        return 0;

    unsigned int count = 0;
    for (u16 y = 0; y < s->height; ++y)
    {
        for (u16 x = 0; x < s->width; ++x)
        {
            size_t idx = (size_t)y * s->width + x;
            u64 id = s->grid[idx];
            if (id != 0)
            {
                cb(x, y, id, user);
                ++count;
            }
        }
    }
    return count;
}