#include "../include/handle.h"
#include "../include/logging.h"
#include <stdlib.h>

/* Internal slot layout. Each slot stores a pointer, a generation counter and an
   active flag. We keep generation as u16 but only expose 9 bits in the handle.
*/

struct handle_table_slot
{
    void *ptr;
    u16 generation; /* increments when slot is freed */
    u16 type : 6;
    u16 active : 1;
};

struct handle_table
{
    u16 capacity;
    u16 count;
    u16 free_head; /* index of first free slot (0 == none). We store indices as 1-based to
                       allow 0 to mean "no free" */
    struct handle_table_slot *slots;
};

handle_table *handle_table_create(u16 capacity)
{
    if (capacity == 0)
        return NULL;

    handle_table *t = (handle_table *)malloc(sizeof(handle_table));
    if (!t)
        return NULL;

    t->slots = (struct handle_table_slot *)calloc(capacity, sizeof(*t->slots));
    if (!t->slots)
    {
        free(t);
        return NULL;
    }

    t->capacity = capacity;
    t->count = 0;
    t->free_head = 0; /* empty free list */

    /* Initially all slots are free; we will not populate an explicit free-list
       since we allocate from 0..capacity-1 until full, afterwards we use free list.
    */

    return t;
}

void handle_table_destroy(handle_table *table)
{
    if (!table)
        return;
    if (table->slots)
        free(table->slots);
    free(table);
}

/* Helpers to build / parse handle32 */

u32 handle_to_u32(handle32 h)
{
    u32 v = 0;
    /* layout: [31..24 unused][23..15 validation(9)][14..9 type(6)][8 active][7..0 index(8?) ]
       But original fixed size: index is u16; we'll pack as: (index << 0) | (validation << 16) | (type << 25) | (active
       << 31) To keep simple and reversible, store fields into a u32 with enough bits:
       - index: bits 0..15 (16 bits)
       - validation: bits 16..24 (9 bits)
       - type: bits 25..30 (6 bits)
       - active: bit 31
    */
    v = ((u32)h.index ^ INVALID_HANDLE_INDEX) | (((u32)h.validation & 0x1FFu) << 16) | (((u32)h.type & 0x3Fu) << 25) |
        (((u32)h.active & 0x1u) << 31);
    return v;
}

handle32 handle_from_u32(u32 v)
{
    handle32 h;
    h.index = (u16)(v ^ INVALID_HANDLE_INDEX);
    h.validation = (u16)((v >> 16) & 0x1FFu);
    h.type = (u16)((v >> 25) & 0x3Fu);
    h.active = (u16)((v >> 31) & 0x1u);
    return h;
}

/* Acquire a slot: return handle with 1-based index? The header expects index be u16; we use 0..capacity-1.
   However we must ensure invalid handle uses 0xFFFF (INVALID_HANDLE_INDEX) as index. */

handle32 handle_table_put(handle_table *table, void *obj, u16 type)
{
    if (!table)
        return INVALID_HANDLE;

    if (type > 0x3F)
        type &= 0x3F; /* clamp to 6 bits */

    u16 slot_index = INVALID_HANDLE_INDEX;

    /* Find free slot: prefer first unused index if capacity not yet reached */
    if (table->count < table->capacity)
    {
        slot_index = table->count; /* 0-based */
        table->count++;
    }
    else
    {
        /* walk slots to find an inactive one */
        for (u16 i = 0; i < table->capacity; ++i)
        {
            if (!table->slots[i].active)
            {
                slot_index = i;
                break;
            }
        }

        if (slot_index == INVALID_HANDLE_INDEX)
        {
            LOG_ERROR("No free slots in a handle table");
            // TODO: expand the table without breaking everything
            return INVALID_HANDLE; /* no free slot */
        }
    }

    struct handle_table_slot *s = &table->slots[slot_index];
    s->ptr = obj;
    s->type = (u16)(type & 0x3F);
    s->active = 1;
    /* generation is left as-is if reused; otherwise default 0 */

    handle32 h;
    h.index = slot_index;
    h.validation = (u16)(s->generation & 0x1FFu);
    h.type = s->type;
    h.active = s->active;
    return h;
}

void handle_table_release(handle_table *table, handle32 h)
{
    if (!table)
        return;
    if (h.index >= table->capacity)
        return;

    struct handle_table_slot *s = &table->slots[h.index];
    /* Only release if handle matches current generation and active */
    if (!s->active)
        return;
    if ((u16)(s->generation & 0x1FFu) != (u16)(h.validation & 0x1FFu))
        return;

    s->active = 0;
    s->ptr = NULL;
    s->generation = (u16)((s->generation + 1) & 0xFFFFu); /* increment generation */
}

void *handle_table_get(handle_table *table, handle32 h)
{
    if (!table)
        return NULL;
    if (h.index == INVALID_HANDLE_INDEX)
        return NULL;
    if (h.index >= table->capacity)
        return NULL;

    struct handle_table_slot *s = &table->slots[h.index];
    if (!s->active)
        return NULL;
    if ((u16)(s->generation & 0x1FFu) != (u16)(h.validation & 0x1FFu))
        return NULL;
    if (s->type != h.type)
        return NULL;

    return s->ptr;
}

bool handle_is_valid(handle_table *table, handle32 h)
{
    return handle_table_get(table, h) != NULL;
}

u16 handle_table_capacity(handle_table *table)
{
    if (!table)
        return 0;
    return table->capacity;
}

void *handle_table_slot_ptr(handle_table *table, u16 index)
{
    if (!table)
        return NULL;
    if (index >= table->capacity)
        return NULL;
    return table->slots[index].ptr;
}

u16 handle_table_slot_active(handle_table *table, u16 index)
{
    if (!table)
        return 0;
    if (index >= table->capacity)
        return 0;
    return (u16)table->slots[index].active;
}

u16 handle_table_slot_generation(handle_table *table, u16 index)
{
    if (!table)
        return 0;
    if (index >= table->capacity)
        return 0;
    return table->slots[index].generation;
}

int handle_table_set_slot(handle_table *table, u16 index, void *ptr, u16 generation, u16 type, u16 active)
{
    if (!table)
        return -1;
    if (index >= table->capacity)
        return -1;
    table->slots[index].ptr = ptr;
    table->slots[index].generation = generation;
    table->slots[index].type = (u16)(type & 0x3F);
    table->slots[index].active = (u16)(active ? 1 : 0);

    /* Ensure the count reflects the highest initialized slot so that future
       allocations behave correctly (we treat `count` as number of slots
       that have been 'touched' for simple reconstruction). */
    if ((u16)(index + 1) > table->count)
        table->count = (u16)(index + 1);

    return 0;
}
