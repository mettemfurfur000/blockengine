#include "include/arena.h"

#include <stdlib.h>

arena *arena_create(u32 size)
{
	void *ret = malloc(sizeof(arena) + size);

	arena *a = ret;
	a->base = ret + sizeof(arena);
	a->capacity = size;
	a->length = 0;

	return a;
}

void arena_destroy(arena *a)
{
	free(a);
}

void *arena_alloc(arena *a, u32 size)
{
	assert(a->length + size <= a->capacity);

	void *ret = a->base + a->length;
	a->length += size;

	return ret;
}

void *arena_get_free_spot(arena *a)
{
	void *ret = a->base + a->length;

	return ret;
}

void arena_free(arena *a)
{
	a->length = 0;
}
