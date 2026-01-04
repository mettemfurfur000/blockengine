#include "include/file_system.h"
#include "include/endianless.h"
#include "include/folder_structure.h"
#include "include/handle.h"
#include "include/level.h"
#include "include/logging.h"
#include "include/vars.h"

/* compression-related stuff */

typedef enum
{
    STREAM_PLAIN, /* ordinary FILE* */
    STREAM_GZIP   /* gzFile from zlib   */
} stream_mode_t;

/* The struct that all functions will actually receive. */
typedef struct
{
    stream_mode_t mode;
    union
    {
        FILE *plain; /* when mode == STREAM_PLAIN */
        gzFile gz;   /* when mode == STREAM_GZIP  */
    } handle;
} stream_t;

/* Compatibility shim – lets us keep the old signatures */
#define AS_STREAM(p) ((stream_t *)(p))

static u8 flip_buf[8] = {};

static inline int stream_write(const void *buf, size_t size, stream_t *s)
{
    if (size % 2 == 0 && size <= 8 && size > 0)
    {
        memcpy(flip_buf, buf, size);
        make_endianless(flip_buf, size);
        buf = flip_buf;
    }

    if (s->mode == STREAM_PLAIN)
    {
        return fwrite(buf, 1, size, s->handle.plain) == size ? 0 : -1;
    }
    else
    { /* STREAM_GZIP */
        int written = gzwrite(s->handle.gz, buf, (unsigned int)size);
        return written == (int)size ? 0 : -1;
    }
}

static inline int stream_read(void *buf, size_t size, stream_t *s)
{
    int ret_code = true;
    if (s->mode == STREAM_PLAIN)
    {
        ret_code = fread(buf, 1, size, s->handle.plain) == size ? 0 : -1;
    }
    else
    { /* STREAM_GZIP */
        int read = gzread(s->handle.gz, buf, (unsigned int)size);
        ret_code = read == (int)size ? 0 : -1;
    }

    if (size % 2 == 0 && size < 9 && size > 0)
        make_endianless(buf, size);

    return ret_code;
}

#define WRITE(object, s)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (stream_write(&(object), sizeof(object), AS_STREAM(s)) != 0)                                                \
        {                                                                                                              \
            LOG_ERROR("Write failed for %s", #object);                                                                 \
        }                                                                                                              \
    } while (0)

#define READ(object, s)                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (stream_read(&(object), sizeof(object), AS_STREAM(s)) != 0)                                                 \
        {                                                                                                              \
            LOG_ERROR("Read failed for %s", #object);                                                                  \
        }                                                                                                              \
    } while (0)

void blob_write(blob b, stream_t *f)
{
    WRITE(b.length, f);
    // fwrite(b.str, 1, b.length, f);
    stream_write(b.str, b.length, f);
}

blob blob_read(stream_t *f)
{
    blob t;
    READ(t.length, f);
    t.str = malloc(t.length);
    stream_read(t.str, t.length, f);
    return t;
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
    READ(b.size, f);
    b.ptr = calloc(b.size, 1);

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

/* Open for writing.  If `compress` is true we create a gzip stream. */
static int stream_open_write(const char *path, int compress, stream_t *out)
{
    if (compress)
    {
        out->mode = STREAM_GZIP;
        out->handle.gz = gzopen(path, "wb9"); /* max compression */
        if (!out->handle.gz)
        {
            LOG_ERROR("gzopen failed for %s", path);
            return -1;
        }
    }
    else
    {
        out->mode = STREAM_PLAIN;
        out->handle.plain = fopen(path, "wb");
        if (!out->handle.plain)
        {
            LOG_ERROR("fopen failed for %s", path);
            return -1;
        }
    }
    return 0;
}

/* Open for reading.  If `compress` is true we expect a gzip stream. */
static int stream_open_read(const char *path, int compress, stream_t *out)
{
    if (compress)
    {
        out->mode = STREAM_GZIP;
        out->handle.gz = gzopen(path, "rb");
        if (!out->handle.gz)
        {
            LOG_ERROR("gzopen failed for %s", path);
            return -1;
        }
    }
    else
    {
        out->mode = STREAM_PLAIN;
        out->handle.plain = fopen(path, "rb");
        if (!out->handle.plain)
        {
            LOG_ERROR("fopen failed for %s", path);
            return -1;
        }
    }
    return 0;
}

/* Close the abstract stream */
static void stream_close(stream_t *s)
{
    if (s->mode == STREAM_PLAIN)
    {
        if (s->handle.plain)
            fclose(s->handle.plain);
    }
    else
    {
        if (s->handle.gz)
            gzclose(s->handle.gz);
    }
}

u8 save_level(level lvl)
{
    char path[256] = {};
    sprintf(path, FOLDER_LVL SEPARATOR_STR "%s.lvl", lvl.name);

    stream_t s;
    if (stream_open_write(path, COMPRESS_LEVELS, &s) != 0)
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
    if (stream_open_read(path, COMPRESS_LEVELS, &s) != 0)
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