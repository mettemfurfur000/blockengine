#ifndef ARENA_H
#define ARENA_H

#include "general.h"

#define STACK_ARR_CAP 16 * 1024

typedef struct
{
	void *base;
	u32 length;
	u32 capacity;
} arena;

// create/destroy on heap
arena *arena_create(u32 size);
void arena_destroy(arena *a);

void *arena_alloc(arena *a, u32 size);
void *arena_get_free_spot(arena *a);
void arena_free(arena *a);

#endif