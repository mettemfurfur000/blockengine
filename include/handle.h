#ifndef HANDLE_H
#define HANDLE_H 1

#include "general.h"

/*
    This file defines the handle data structure used throughout the engine.
*/

typedef struct
{
    u16 index;           // Unique identifier for the handle
    u16 validation : 15; // Validation bits for the handle
    u16 active : 1;      // Is the handle active?
} handle32;

static_assert(sizeof(handle32) == sizeof(u32), "");

/* Invalid handle helper */
#define INVALID_HANDLE_INDEX 0xFFFFu
#define INVALID_HANDLE (handle32){.index = INVALID_HANDLE_INDEX, .validation = 0, .active = 0}

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct handle_table handle_table;

#define MAX_HANDLE_TABLE_CAPACITY (u16) - 1

    /* Create / destroy a handle table for up to `capacity` slots. Returns NULL on OOM. */
    handle_table *handle_table_create(u16 capacity);
    void handle_table_destroy(handle_table *table);

    /* Acquire a slot for `obj`. `type` is a 6-bit user tag (0..63). Returns an invalid
         handle (index == 0xFFFF) on failure. */
    handle32 handle_table_put(handle_table *table, void *obj);

    /* Release a previously acquired handle. If the handle is invalid or already freed
         this is a no-op. */
    void handle_table_release(handle_table *table, handle32 h);

    /* Return the stored pointer for a handle, or NULL if the handle is invalid. */
    void *handle_table_get(handle_table *table, handle32 h);

    /* Query whether a handle is valid (refers to a currently active slot). */
    bool handle_is_valid(handle_table *table, handle32 h);

    /* Additional helpers for advanced use (inspection / serialization). */
    /* Return the configured capacity (number of slots) for the table. */
    u16 handle_table_capacity(handle_table *table);

    /* Return pointer stored in slot `index` (may be NULL). This accesses the raw slot
        pointer regardless of the slot's active flag - useful for saving/restoring state. */
    void *handle_table_slot_ptr(handle_table *table, u16 index);

    /* Return whether the slot `index` is currently active (1) or not (0). */
    u16 handle_table_slot_active(handle_table *table, u16 index);

    /* Return the generation stored for the slot (raw u16). Useful to preserve handle
        validation values when deserializing. */
    u16 handle_table_slot_generation(handle_table *table, u16 index);

    /* Set a slot's raw contents - used during deserialization to reconstruct exact
        handle values. Returns 0 on success, non-zero on failure. */
    int handle_table_set_slot(handle_table *table, u16 index, void *ptr, u16 generation, u16 active);

    /* Returns first unused slot, or invalid handle if all are taken */
    u16 handle_table_get_inactive(handle_table *table);

#ifdef __cplusplus
}
#endif

#endif