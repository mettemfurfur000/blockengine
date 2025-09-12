#include "include/file_system.h"
#include "include/endianless.h"
#include "include/folder_structure.h"
#include "include/level.h"
#include "include/vars.h"

#define WRITE(object, f) endianless_write((u8 *)&object, sizeof(object), f)
#define READ(object, f) endianless_read((u8 *)&object, sizeof(object), f)

void blob_write(blob b, FILE *f)
{
    WRITE(b.length, f);
    fwrite(b.str, 1, b.length, f);
}

blob blob_read(FILE *f)
{
    blob t;
    READ(t.length, f);
    t.str = malloc(t.length);
    fread(t.str, 1, t.length, f);
    return t;
}

void blob_vars_write(blob b, FILE *f)
{
    WRITE(b.length, f);

    VAR_FOREACH(b, {
        putc(b.ptr[i], f);
        u8 size = b.ptr[i + 1];
        putc(size, f);
        u8 *val = VAR_VALUE(b.ptr, i);
        if (size % 2 == 0 && size <= 8 && size > 0)
            endianless_write(val, size, f);
        else
            fwrite(val, 1, size, f);
    });
}

blob blob_vars_read(FILE *f)
{
    blob b;
    READ(b.size, f);
    b.ptr = calloc(b.size, 1);

    assert(b.ptr != NULL);

    VAR_FOREACH(b, {
        b.ptr[i] = getc(f);
        u8 size = getc(f);
        b.ptr[i + 1] = size;
        u8 *val = VAR_VALUE(b.ptr, i);
        if (size % 2 == 0 && size <= 8 && size > 0)
            endianless_read(val, size, f);
        else
            fread(val, 1, size, f);
    });

    return b;
}

void write_hashtable(hash_node **t, FILE *f)
{
    const u32 size = TABLE_SIZE;
    const u32 elements = table_elements(t);

    WRITE(size, f);     // write table size
    WRITE(elements, f); // write number of elements

    for (u32 i = 0; i < TABLE_SIZE; i++)
    {
        hash_node *n = t[i];
        while (n != NULL)
        {
            blob_write(n->key, f);
            blob_write(n->value, f);

            n = n->next;
        }
    }
}

void read_hashtable(hash_node **t, FILE *f)
{
    u32 size = TABLE_SIZE;
    u32 elements = 0;

    READ(size, f);
    READ(elements, f);

    if (size != TABLE_SIZE)
        LOG_WARNING("Table size mismatch, expected %d, got %d", TABLE_SIZE, size);

    for (u32 i = 0; i < elements; i++)
    {
        blob key = blob_read(f);
        blob value = blob_read(f);

        put_entry(t, key, value); // i could just put it in place but im not
                                  // sure about the order of them being called
    }
}

void write_layer(layer *l, FILE *f)
{
    WRITE(l->block_size, f);
    WRITE(l->var_index_size, f);
    WRITE(l->flags, f);
    WRITE(l->width, f);
    WRITE(l->height, f);
    WRITE(l->uuid, f);

    if (!l->registry)
        blob_write(blobify("no_registry"), f);
    else
        blob_write(blobify((char *)l->registry->name), f);

    u64 id = 0;
    u32 index = 0;
    for (u32 y = 0; y < l->height; y++)
        for (u32 x = 0; x < l->width; x++)
        {
            if (block_get_id(l, x, y, &id) != SUCCESS)
            {
                LOG_ERROR("failed to read the block id on %d %d", x, y);
                return;
            }
            endianless_write((u8 *)&id, l->block_size, f);
            index = block_get_vars_index(l, x, y);
            endianless_write((u8 *)&index, l->var_index_size, f);
        }

    // layer_clean_vars(l);

    /* Serialize handle table: write capacity (slot count) and for each slot write
       active flag and blob contents (or zero blob for empty slots). */
    if (!l->var_pool.table)
    {
        u32 zero32 = 0;
        WRITE(zero32, f);
    }
    else
    {
        u16 cap = handle_table_capacity(l->var_pool.table);
        u32 cap32 = (u32)cap;
        WRITE(cap32, f);
        for (u16 i = 0; i < cap; ++i)
        {
            void *p = handle_table_slot_ptr(l->var_pool.table, i);
            u16 active = handle_table_slot_active(l->var_pool.table, i);
            WRITE(active, f);
            if (p)
            {
                blob_vars_write(*(blob *)p, f);
            }
            else
            {
                /* write empty blob */
                u32 zero32 = 0;
                WRITE(zero32, f);
            }
        }
    }
}

void read_layer(layer *l, room *parent, FILE *f)
{
    READ(l->block_size, f);
    READ(l->var_index_size, f);

    l->total_bytes_per_block = l->block_size + l->var_index_size;

    READ(l->flags, f);
    READ(l->width, f);
    READ(l->height, f);
    READ(l->uuid, f);

    blob registry_name = blob_read(f);

    if (registry_name.length == 0)
        l->registry = NULL;
    else
    {
        if (strcmp(registry_name.str, "no_registry") != 0)
        {
            l->registry = find_registry(((level *)parent->parent_level)->registries, registry_name.str);

            if (!l->registry)
                LOG_WARNING("Registry %s not found", registry_name.str);
        }

        free(registry_name.str);
    }

    init_layer(l, parent);

    u64 id = 0;
    for (u32 y = 0; y < l->height; y++)
        for (u32 x = 0; x < l->width; x++)
        {
            endianless_read((u8 *)&id, l->block_size, f); // read id

            if (block_set_id(l, x, y, id) != SUCCESS)
            {
                LOG_WARNING("Failed to read block at %d, %d", x, y);
                block_set_id(l, x, y, 0);
            }

            endianless_read((u8 *)&id, l->var_index_size, f); // read var index
            block_var_index_set(l, x, y, id);
        }

    u32 slot_count = 0;
    READ(slot_count, f);
    if (slot_count == 0)
    {
        /* no table */
    }
    else
    {
        /* create a handle table with slot_count capacity and fill slots */
        l->var_pool.table = handle_table_create((u16)slot_count);
        l->var_pool.type_tag = 1; /* same tag used when writing */

        for (u16 i = 0; i < (u16)slot_count; ++i)
        {
            u16 active = 0;
            READ(active, f);
            blob b = blob_vars_read(f);
            if (b.size == 0 || b.ptr == NULL)
            {
                /* empty slot */
                handle_table_set_slot(l->var_pool.table, i, NULL, 0, 0, 0);
            }
            else
            {
                /* allocate new blob and set raw slot */
                blob *nb = calloc(1, sizeof(blob));
                assert(nb != NULL);

                nb->ptr = calloc(b.size ? b.size : 1, 1);
                assert(nb->ptr != NULL);
            
                if (b.size && b.ptr)
                    memcpy(nb->ptr, b.ptr, b.size);
                nb->size = b.size;
                /* generation left at 0; type set to pool tag; active follows `active` */
                handle_table_set_slot(l->var_pool.table, i, nb, 0, l->var_pool.type_tag, active);
            }
        }
    }
}

void write_room(room *r, FILE *f)
{
    WRITE(r->uuid, f);
    WRITE(r->width, f);
    WRITE(r->height, f);

    CHECK_PTR_NORET(r->name)
    blob_write(blobify(r->name), f);

    WRITE(r->layers.length, f);

    for (u32 i = 0; i < r->layers.length; i++)
        write_layer(r->layers.data[i], f);
}

void read_room(room *r, FILE *f)
{
    READ(r->uuid, f);
    READ(r->width, f);
    READ(r->height, f);

    r->name = blob_read(f).str;

    READ(r->layers.length, f);
    vec_reserve(&r->layers, r->layers.length);

    for (u32 i = 0; i < r->layers.length; i++)
    {
        layer *l = calloc(1, sizeof(layer));
        assert(l != NULL);

        read_layer(l, r, f);
        r->layers.data[i] = l;
    }
}

u8 save_level(level lvl)
{
    char path[256] = {};
    sprintf(path, FOLDER_LVL SEPARATOR_STR "%s.lvl", lvl.name);
    FILE *f = fopen(path, "wb");
    if (!f)
        return FAIL;

    // CHECK_PTR(lvl.name)
    if (!lvl.name)
    {
        fclose(f);
        LOG_ERROR("Level name is null");
        return FAIL;
    }

    WRITE(lvl.uuid, f);
    blob_write(blobify(lvl.name), f);

    WRITE(lvl.registries.length, f);
    for (u32 i = 0; i < lvl.registries.length; i++)
    {
        const char *reg_name = ((block_registry *)lvl.registries.data[i])->name;
        CHECK_PTR(reg_name)
        blob_write(blobify((char *)reg_name), f); // only save registry names
    }

    WRITE(lvl.rooms.length, f);
    for (u32 i = 0; i < lvl.rooms.length; i++)
        write_room(lvl.rooms.data[i], f);

    fclose(f);

    return SUCCESS;
}

u8 load_level(level *lvl, const char *name_in)
{
    char path[256] = {};
    sprintf(path, FOLDER_LVL SEPARATOR_STR "%s.lvl", name_in);
    FILE *f = fopen(path, "rb");
    if (!f)
        return FAIL;

    READ(lvl->uuid, f);
    lvl->name = blob_read(f).str;

    u32 reg_count;
    READ(reg_count, f);
    for (u32 i = 0; i < reg_count; i++)
    {
        char *name = blob_read(f).str;
        block_registry *reg = calloc(1, sizeof(block_registry));

        assert(reg != NULL);

        reg->name = name;

        // sprintf(path, FOLDER_REG SEPARATOR_STR "%s", name);

        if (read_block_registry(reg, name) != SUCCESS)
        {
            LOG_WARNING("Failed to load registry %s", name);
            free(name);
            continue;
        }

        (void)vec_push(&lvl->registries, reg);
        // free(name);
    }

    u32 room_count;
    READ(room_count, f);
    for (u32 i = 0; i < room_count; i++)
    {
        room *new_room = calloc(1, sizeof(room));

        assert(new_room != NULL);

        new_room->parent_level = lvl;
        read_room(new_room, f);
        (void)vec_push(&lvl->rooms, new_room);
    }

    fclose(f);

    return SUCCESS;
}
