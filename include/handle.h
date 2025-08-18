#ifndef HANDLE_H
#define HANDLE_H 1

#include "general.h"

/*
    This file defines the handle data structure used throughout the engine.
*/

typedef struct {
    u16 index;          // Unique identifier for the handle
    u16 validation : 9; // Validation bits for the handle
    u16 type : 6;       // Type of the handle
    u16 active : 1;     // Is the handle active?
} handle32;

static_assert(sizeof(handle32) == sizeof(u32), "");

/* Invalid handle helper */
#define INVALID_HANDLE_INDEX 0xFFFFu
#define INVALID_HANDLE (handle32){.index = INVALID_HANDLE_INDEX, .validation = 0, .type = 0, .active = 0}

/*
        Generic handle table API.
        - Stores pointers to objects and a small generation counter per slot.
        - A returned `handle32` encodes the slot index, the validation generation,
            a 6-bit type tag (user provided) and an active bit.
        - Use `handle_table_put` to allocate a slot and get a handle.
        - Use `handle_table_get` to retrieve the stored pointer for a handle (NULL if invalid).
        - Use `handle_table_release` to free the slot; this bumps the generation so stale
            handles become invalid.

        The implementation limits the generation (validation) to 9 bits and type to 6 bits
        as encoded in `handle32`.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct handle_table handle_table;

/* Create / destroy a handle table for up to `capacity` slots. Returns NULL on OOM. */
handle_table *handle_table_create(u16 capacity);
void handle_table_destroy(handle_table *table);

/* Acquire a slot for `obj`. `type` is a 6-bit user tag (0..63). Returns an invalid
     handle (index == 0xFFFF) on failure. */
handle32 handle_table_put(handle_table *table, void *obj, u16 type);

/* Release a previously acquired handle. If the handle is invalid or already freed
     this is a no-op. */
void handle_table_release(handle_table *table, handle32 h);

/* Return the stored pointer for a handle, or NULL if the handle is invalid. */
void *handle_table_get(handle_table *table, handle32 h);

/* Query whether a handle is valid (refers to a currently active slot). */
bool handle_is_valid(handle_table *table, handle32 h);

/* Helpers to convert to/from a u32 representation (useful for serialization). */
u32 handle_to_u32(handle32 h);
handle32 handle_from_u32(u32 v);

#ifdef __cplusplus
}
#endif

#endif