#include "../include/file_system.h"
#include "../include/level.h"

const char *levels_folder = "levels";

#define WRITE(object, f) endianless_write((u8 *)&object, sizeof(object), f)
#define READ(object, f) endianless_read((u8 *)&object, sizeof(object), f)

void blob_write(blob b, FILE *f)
{
    WRITE(b.length, f);
    fwrite(b.str, 1, b.length, f);
}

void blob_read(blob *b, FILE *f)
{
    READ(b->length, f);
    b->str = malloc(b->length);
    fread(b->str, 1, b->length, f);
}

void write_room(room *r, FILE *f)
{
    WRITE(r->width, f);
    WRITE(r->height, f);
    WRITE(r->depth, f);

    blob_write(blobify(r->name), f);

    const u32 total_blocks = r->height + r->width;

    for (u32 i = 0; i < r->depth; i++)
    {
        layer layer = r->layers.data[i];

        WRITE(layer.bytes_per_block, f);
        WRITE(layer.flags, f);

        if (layer.registry != 0)
            blob_write(blobify((char *)layer.registry->name), f);
        else
            blob_write(blobify(""), f);

        for (u32 x = 0; x < r->width; x++)
            for (u32 y = 0; y < r->height; y++)
            {
                u64 id = 0;
                block_get_id(r, i, x, y, &id);
                endianless_write((u8 *)&id, layer.bytes_per_block, f);
            }
    }
}

void read_room(level *parent_level, room *r, FILE *f)
{
    READ(r->width, f);
    READ(r->height, f);
    READ(r->depth, f);

    blob t = {};

    blob_read(&t, f);
    r->name = t.str;

    const u32 total_blocks = r->height + r->width;

    for (u32 i = 0; i < r->depth; i++)
    {
        layer l;
        blob reg_name = {};

        READ(l.bytes_per_block, f);
        READ(l.flags, f);

        blob_read(&reg_name, f);

        if (FLAG_GET(l.flags, LAYER_FLAG_HAS_REGISTRY))
            if (!(l.registry = find_registry(&parent_level->registries, reg_name.str)))
            {
                LOG_ERROR("Could not find registry '%s' for room '%s', defaulting to no-registry mode", reg_name.str, r->name);
                FLAG_SET(l.flags, LAYER_FLAG_HAS_REGISTRY, 0);
            }

        alloc_layer(&l, r);

        vec_push(&r->layers, l);
    }
}

int save_level_to_file(level *l)
{
    return SUCCESS;
}

// Level loading functions
int load_level_from_file(level *l)
{
}

// Utility functions
int check_file_exists(const char *filename)
{
}

int create_levels_directory(void)
{
}