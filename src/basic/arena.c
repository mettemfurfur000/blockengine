#include "include/arena.h"

#include <stdlib.h>
#include <string.h>

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

bool arena_grow(arena *a, u32 new_capacity)
{
	if (new_capacity <= a->capacity)
		return true; // Already big enough

	// Allocate new block
	void *new_block = malloc(sizeof(arena) + new_capacity);
	if (!new_block)
		return false;

	// Copy existing data
	memcpy(new_block + sizeof(arena), a->base, a->length);

	// Free old block and update arena
	void *old_base = a->base;
	a->base = new_block + sizeof(arena);
	a->capacity = new_capacity;

	// We need to free the old arena wrapper too
	// Find the start of the old arena (it's sizeof(arena) bytes before a->base)
	void *old_arena_start = old_base - sizeof(arena);
	free(old_arena_start);

	return true;
}

void *arena_alloc(arena *a, u32 size)
{
	if (a->length + size > a->capacity)
	{
		// Try to grow
		u32 new_capacity = (a->capacity * 2) > (a->capacity + size) ? (a->capacity * 2) : (a->capacity + size + 8192);
		if (!arena_grow(a, new_capacity))
			return NULL;
	}

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
