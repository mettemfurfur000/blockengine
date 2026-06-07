#include "include/arena.h"

#include <stdlib.h>
#include <string.h>

/****************************************
 * Arena implementation that preserves
 * old backing buffers when growing so
 * pointers returned earlier remain valid.
 *
 * Strategy:
 * - Allocate arena struct separately from
 *   its backing memory block.
 * - On grow, allocate a new backing block,
 *   copy existing data into it, and keep
 *   the old backing pointer in a list so it
 *   is not freed until arena_destroy.
 ****************************************/

typedef struct old_block_node
{
	void *ptr;
	struct old_block_node *next;
} old_block_node;

typedef struct arena_record
{
	arena *owner;
	old_block_node *old_blocks;
	struct arena_record *next;
} arena_record;

static arena_record *g_arena_records = NULL;

static arena_record *get_record(arena *a)
{
	arena_record *r = g_arena_records;
	while (r)
	{
		if (r->owner == a)
			return r;
		r = r->next;
	}

	r = (arena_record *)malloc(sizeof(arena_record));
	if (!r)
		return NULL;
	r->owner = a;
	r->old_blocks = NULL;
	r->next = g_arena_records;
	g_arena_records = r;
	return r;
}

static void remove_record(arena *a)
{
	arena_record **prev = &g_arena_records;
	arena_record *r = g_arena_records;
	while (r)
	{
		if (r->owner == a)
		{
			*prev = r->next;
			// free nodes
			old_block_node *n = r->old_blocks;
			while (n)
			{
				old_block_node *nx = n->next;
				free(n->ptr);
				free(n);
				n = nx;
			}
			free(r);
			return;
		}
		prev = &r->next;
		r = r->next;
	}
}

arena *arena_create(u32 size)
{
	arena *a = (arena *)malloc(sizeof(arena));
	if (!a)
		return NULL;

	a->base = malloc(size);
	if (!a->base)
	{
		free(a);
		return NULL;
	}

	a->capacity = size;
	a->length = 0;

	// register arena so we can keep old backing blocks alive
	get_record(a);

	return a;
}

void arena_destroy(arena *a)
{
	if (!a)
		return;

	// remove record (frees all old blocks)
	remove_record(a);

	// free current backing block and arena struct
	free(a->base);
	free(a);
}

bool arena_grow(arena *a, u32 new_capacity)
{
	if (new_capacity <= a->capacity)
		return true; // Already big enough

	void *new_block = malloc(new_capacity);
	if (!new_block)
		return false;

	// Copy existing data into new block
	memcpy(new_block, a->base, a->length);

	// Keep old block alive so pointers into it remain valid
	arena_record *r = get_record(a);
	if (r)
	{
		old_block_node *n = (old_block_node *)malloc(sizeof(old_block_node));
		if (n)
		{
			n->ptr = a->base;
			n->next = r->old_blocks;
			r->old_blocks = n;
		}
	}

	// switch to new block
	a->base = new_block;
	a->capacity = new_capacity;

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

	void *ret = (u8 *)a->base + a->length;
	a->length += size;

	return ret;
}

void *arena_get_free_spot(arena *a)
{
	void *ret = (u8 *)a->base + a->length;

	return ret;
}

void arena_free(arena *a)
{
	a->length = 0;
}
