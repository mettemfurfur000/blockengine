#include "../include/file_system.h"
#include "../include/level.h"

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
        if (size % 2 == 0)
            endianless_write(val, size, f);
        else
            fwrite(val, 1, size, f);
    });
}

blob blob_vars_read(blob b, FILE *f)
{
    blob t;
    READ(t.size, f);
    t.ptr = calloc(t.size, 1);

    VAR_FOREACH(b, {
        b.ptr[i] = getc(f);
        u8 size = getc(f);
        b.ptr[i + 1] = size;
        u8 *val = VAR_VALUE(b.ptr, i);
        if (size % 2 == 0)
            endianless_read(val, size, f);
        else
            fread(val, 1, size, f);
    });

    return t;
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

        put_entry(t, key, value); // i could just put it in place but im not sure about the order of them being called
    }
}

void write_layer(layer l, level lvl, FILE *f)
{
    WRITE(l.bytes_per_block, f);
    WRITE(l.flags, f);

    if (!l.registry)
        blob_write(blobify("no_registry"), f);
    else
        blob_write(blobify((char *)l.registry->name), f);

    u64 id = 0;
    for (u32 x = 0; x < l.width; x++)
        for (u32 y = 0; y < l.height; y++)
        {
            if (block_get_id(&l, x, y, &id) == FAIL)
                continue;
            endianless_write((u8 *)&id, l.bytes_per_block, f);
        }

    write_hashtable(l.vars, f);
}

void read_layer(layer *l, level lvl, FILE *f)
{
    READ(l->bytes_per_block, f);
    READ(l->flags, f);

    blob registry_name = blob_read(f);

    if (registry_name.length == 0)
        l->registry = NULL;
    else
    {
        l->registry = find_registry(lvl.registries, registry_name.str);

        if (!l->registry)
            LOG_WARNING("Registry %s not found", registry_name.str);

        free(registry_name.str);
    }

    u64 id = 0;
    for (u32 x = 0; x < l->width; x++)
        for (u32 y = 0; y < l->height; y++)
        {
            if (block_get_id(l, x, y, &id) == FAIL)
            {
                LOG_WARNING("Failed to read block at %d, %d", x, y);
                block_set_id(l, x, y, 0);
                continue;
            }
            block_set_id(l, x, y, id);
        }

    read_hashtable(l->vars, f);
}

void write_room(room *r, level lvl, FILE *f)
{
    WRITE(r->width, f);
    WRITE(r->height, f);

    CHECK_PTR_NORET(r->name)
    blob_write(blobify(r->name), f);

    for (u32 i = 0; i < r->layers.length; i++)
        write_layer(r->layers.data[i], lvl, f);
}

void read_room(room *r, level lvl, FILE *f)
{
    READ(r->width, f);
    READ(r->height, f);

    r->name = blob_read(f).str;

    layer tmp;

    for (u32 i = 0; i < r->layers.length; i++)
    {
        read_layer(&tmp, lvl, f);
        (void)vec_push(&r->layers, tmp);
    }
}

u8 save_level(level lvl)
{
    char path[256] = {};
    sprintf(path, LEVELS_FOLDER "/%s.lvl", lvl.name);
    FILE *f = fopen(path, "wb");
    if (!f)
        return FAIL;

    CHECK_PTR(lvl.name)

    blob_write(blobify(lvl.name), f);
    WRITE(lvl.registries.length, f);
    for (u32 i = 0; i < lvl.registries.length; i++)
    {
        const char *reg_name = lvl.registries.data[i].name;
        CHECK_PTR(reg_name)
        blob_write(blobify((char *)reg_name), f); // only save registry names
    }

    WRITE(lvl.rooms.length, f);
    for (u32 i = 0; i < lvl.rooms.length; i++)
        write_room(&lvl.rooms.data[i], lvl, f);

    fclose(f);

    return SUCCESS;
}

u8 load_level(level *lvl, char *name)
{
    char path[256] = {};
    sprintf(path, LEVELS_FOLDER "/%s.lvl", name);
    FILE *f = fopen(path, "rb");
    if (!f)
        return FAIL;

    lvl->name = blob_read(f).str;

    u32 reg_count;
    READ(reg_count, f);
    for (u32 i = 0; i < reg_count; i++)
    {
        char *name = blob_read(f).str;
        block_registry reg = {.name = name};

        sprintf(path, REGISTRIES_FOLDER "/%s", name);

        if (read_block_registry(path, &reg) == FAIL)
        {
            LOG_WARNING("Failed to load registry %s", name);
            free(name);
            continue;
        }

        (void)vec_push(&lvl->registries, reg);
        free(name);
    }

    u32 room_count;
    READ(room_count, f);
    for (u32 i = 0; i < room_count; i++)
    {
        room tmp = {};
        read_room(&tmp, *lvl, f);
        (void)vec_push(&lvl->rooms, tmp);
    }

    fclose(f);

    return SUCCESS;
}
