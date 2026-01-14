#include "include/file_system.h"
#include "include/folder_structure.h"
#include "include/handle.h"
#include "include/level.h"
#include "include/logging.h"
#include "include/vars.h"

u8 flip_buf[8] = {};

void blob_write(blob b, stream_t *f)
{
    WRITE(b.length, f);
    // fwrite(b.str, 1, b.length, f);
    stream_write(b.str, b.length, f);
}

blob blob_read(stream_t *f)
{
    blob b;
    READ(b.length, f);
    b.str = malloc(b.length);
    stream_read(b.str, b.length, f);
    return b;
}

void blob_vars_write(blob b, stream_t *f)
{
    WRITE(b.length, f);

    // stream_write(b.ptr, b.length, f);

    VAR_FOREACH(b, {
        char letter = b.ptr[i];
        u8 size = b.ptr[i + 1];
        u8 *val = b.ptr + i + 2;

        stream_write(&letter, 1, f);
        stream_write(&size, 1, f);
        stream_write(val, size, f);
    });
}

blob blob_vars_read(stream_t *f)
{
    blob b;
    READ(b.length, f);
    b.ptr = calloc(b.length, 1);

    assert(b.ptr != NULL);

    VAR_FOREACH(b, {
        char letter;
        u8 size;

        stream_read(&letter, 1, f);
        stream_read(&size, 1, f);
        b.ptr[i] = (u8)letter;
        b.ptr[i + 1] = size;
        stream_read(b.ptr + i + 2, size, f);
    });

    return b;
}

void write_hashtable(hash_node **t, stream_t *f)
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

void read_hashtable(hash_node **t, stream_t *f)
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

void stream_embed_file_write(const char *file_path, stream_t *s)
{
    // open the file
    FILE *file = fopen(file_path, "rb");
    // get file size
    fseek(file, 0, SEEK_END);
    u32 file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    // write file size
    WRITE(file_size, s);

    // read and write file data in chunks
    u8 buffer[BUFFER_SIZE];
    u32 bytes_remaining = file_size;
    while (bytes_remaining > 0)
    {
        u32 bytes_to_read = bytes_remaining < sizeof(buffer) ? bytes_remaining : sizeof(buffer);
        size_t read_bytes = fread(buffer, 1, bytes_to_read, file);
        if (read_bytes == 0)
        {
            LOG_ERROR("Failed to read from file during embedding: %s", file_path);
            break;
        }
        WRITE_N(buffer, s, read_bytes);
        bytes_remaining -= (u32)read_bytes;
    }

    fclose(file);
}

void stream_embed_file_read_to_mem(stream_t *s, u8 **out_data, u32 *out_size)
{
    // read file size
    u32 file_size = 0;
    READ(file_size, s);
    *out_size = file_size;

    // allocate memory
    u8 *data = malloc(file_size);
    assert(data != NULL);

    // read file data in chunks
    u32 bytes_remaining = file_size;
    u8 *data_ptr = data;
    while (bytes_remaining > 0)
    {
        u32 bytes_to_read = bytes_remaining < BUFFER_SIZE ? bytes_remaining : BUFFER_SIZE;
        READ_N(data_ptr, s, bytes_to_read);
        data_ptr += bytes_to_read;
        bytes_remaining -= bytes_to_read;
    }

    *out_data = data;
}

// TODO: this serialization code is stinky, mayb remake in a more modular way?/

void write_layer(layer *l, stream_t *f)
{
    WRITE(l->block_size, f);
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
            stream_write(&id, l->block_size, f);
            index = block_get_vars_index(l, x, y);
            stream_write(&index, sizeof(handle32), f);
        }

    /* Serialize handle table: write capacity (slot count) and for each slot write
       active flag and blob contents (or zero blob for empty slots). */
    if (!l->var_pool.table)
    {
        u32 zero32 = 0;
        WRITE(zero32, f);
    }
    else
    {
        // u16 debug_active_amount = 0;
        u16 cap = handle_table_capacity(l->var_pool.table);
        u32 cap32 = (u32)cap;
        WRITE(cap32, f);
        for (u16 i = 0; i < cap; ++i)
        {
            void *p = handle_table_slot_ptr(l->var_pool.table, i);
            u8 active = handle_table_slot_active(l->var_pool.table, i);
            WRITE(active, f);
            u16 generation = handle_table_slot_generation(l->var_pool.table, i);
            WRITE(generation, f);
            if (p)
            {
                // debug_active_amount++;
                blob_vars_write(*(blob *)p, f);
            }
            else
            {
                /* write empty blob */
                u32 zero32 = 0;
                WRITE(zero32, f);
            }
        }
        // LOG_DEBUG("total handles saved: %d out of %d", debug_active_amount, cap);
    }
}

void read_layer(layer *l, room *parent, stream_t *f)
{
    READ(l->block_size, f);

    l->total_bytes_per_block = l->block_size + sizeof(handle32);

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
    u32 index = 0;
    for (u32 y = 0; y < l->height; y++)
        for (u32 x = 0; x < l->width; x++)
        {
            stream_read((u8 *)&id, l->block_size, f); // read id

            if (block_set_id(l, x, y, id) != SUCCESS)
            {
                LOG_WARNING("Failed to read block at %d, %d", x, y);
                block_set_id(l, x, y, 0);
            }

            stream_read((u8 *)&index, sizeof(handle32), f); // read var index
            block_var_index_set(l, x, y, index);
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
        // l->var_pool.type_tag = 1; /* same tag used when writing */

        // u16 debug_active_amount = 0;

        for (u16 i = 0; i < (u16)slot_count; ++i)
        {
            u8 active = 0;
            READ(active, f);
            u16 generation = 0;
            READ(generation, f);
            blob b = blob_vars_read(f);
            if (b.size == 0 || b.ptr == NULL)
            {
                /* empty slot */
                handle_table_set_slot(l->var_pool.table, i, NULL, 0, 0);
            }
            else
            {
                /* allocate new blob and set raw slot */
                blob *nb = calloc(1, sizeof(blob));
                assert(nb != NULL);

                nb->ptr = b.ptr;
                nb->size = b.size;

                handle_table_set_slot(l->var_pool.table, i, nb, generation, active);
                // debug_active_amount++;
            }
        }
        // LOG_DEBUG("restored %d active var handles", debug_active_amount);
    }
}

void write_room(room *r, stream_t *f)
{
    WRITE(r->uuid, f);
    WRITE(r->width, f);
    WRITE(r->height, f);

    assert(r->name);
    blob_write(blobify(r->name), f);

    WRITE(r->layers.length, f);

    for (u32 i = 0; i < r->layers.length; i++)
        write_layer(r->layers.data[i], f);
}

void read_room(room *r, stream_t *f)
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

    stream_t s;
    if (stream_open_write(path, COMPRESS_LEVEL, &s) != 0)
        return FAIL;

    /* The rest of the function stays exactly the same – just pass &s instead of FILE* */
    WRITE(lvl.uuid, &s);
    blob_write(blobify(lvl.name), &s);

    WRITE(lvl.registries.length, &s);
    for (u32 i = 0; i < lvl.registries.length; i++)
    {
        const char *reg_name = ((block_registry *)lvl.registries.data[i])->name;
        assert(reg_name);
        blob_write(blobify((char *)reg_name), &s);
    }

    WRITE(lvl.rooms.length, &s);
    for (u32 i = 0; i < lvl.rooms.length; i++)
        write_room(lvl.rooms.data[i], &s);

    stream_close(&s);
    return SUCCESS;
}

u8 load_level(level *lvl, const char *name_in)
{
    char path[256] = {};
    sprintf(path, FOLDER_LVL SEPARATOR_STR "%s.lvl", name_in);

    stream_t s;
    if (stream_open_read(path, COMPRESS_LEVEL, &s) != 0)
        return FAIL;

    READ(lvl->uuid, &s);
    lvl->name = blob_read(&s).str;

    u32 reg_count;
    READ(reg_count, &s);
    for (u32 i = 0; i < reg_count; i++)
    {
        char *name = blob_read(&s).str;
        block_registry *reg = calloc(1, sizeof(block_registry));
        assert(reg != NULL);
        reg->name = name;
        if (read_block_registry(reg, name) != SUCCESS)
        {
            LOG_WARNING("Failed to load registry %s", name);
            free(name);
            continue;
        }
        (void)vec_push(&lvl->registries, reg);
    }

    u32 room_count;
    READ(room_count, &s);
    for (u32 i = 0; i < room_count; i++)
    {
        room *new_room = calloc(1, sizeof(room));
        assert(new_room != NULL);
        new_room->parent_level = lvl;
        read_room(new_room, &s);
        (void)vec_push(&lvl->rooms, new_room);
    }

    stream_close(&s);
    return SUCCESS;
}