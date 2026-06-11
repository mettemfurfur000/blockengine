#ifndef MULTIBLOCK_ENTITY_H
#define MULTIBLOCK_ENTITY_H 1

#include "general.h"

#ifdef __cplusplus
extern "C" {
#endif

// Per-block physics properties (optional override)
typedef struct block_physics_prop
{
    u64 block_id;       // optional id this prop targets (0 = any)
    float density;
    float friction;
    float restitution;
    u8 disabled; // treat as void / sensor if non-zero
} block_physics_prop;

// Grid-based multiblock shape (row-major)
typedef struct multiblock_shape
{
    u16 width;
    u16 height;
    // width * height entries, row-major. 0 = void
    u64 *grid;

    // Optional per-block physics properties (parallel array or sparse)
    block_physics_prop *props;
    u32 props_count;
} multiblock_shape;

// Allocate a multiblock shape. Copies the provided grid (if non-NULL).
multiblock_shape *multiblock_shape_create(u16 width, u16 height, const u64 *grid);

// Free shape memory
void multiblock_shape_destroy(multiblock_shape *s);

// Get block id at x,y (returns 0 on out-of-bounds or void)
u64 multiblock_shape_get(const multiblock_shape *s, u16 x, u16 y);

// Iterate non-zero blocks; callback receives x,y,id and user pointer. Returns number of items visited.
unsigned int multiblock_shape_iterate_nonzero(const multiblock_shape *s, void (*cb)(u16 x, u16 y, u64 id, void *user), void *user);

#ifdef __cplusplus
}
#endif

#endif // MULTIBLOCK_ENTITY_H
