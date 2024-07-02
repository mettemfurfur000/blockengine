#ifndef WORLD_UTILS_H
#define WORLD_UTILS_H

#include "block_operations.h"
#include "block_registry.h"
#include "data_manipulations.h"

void bprintf(const world *w, block_registry_t *reg, int layer, int orig_x, int orig_y, int length_limit, const char *format, ...);

#endif