#include "memory_control_functions.c"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const int bigendianchecker = 1;
bool is_bigendian() { return (*(char *)&bigendianchecker) == 0; };

void write_block(block *b, FILE *f)
{
    fwrite(&b->id, sizeof(int), 1, f);
    fwrite(&b->data_size, sizeof(int), 1, f);
    fwrite(b->data, 1, b->data_size, f);
}

void read_block(block *b, FILE *f)
{
    fread(&b->id, sizeof(int), 1, f);
    fread(&b->data_size, sizeof(int), 1, f);
    b->data = (byte *)calloc(b->data_size, 1);
    fread(b->data, 1, b->data_size, f);
}

int write_chunk(layer_chunk *c, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f)
        return 0;
    for (int i = 0; i < c->width; i++)
    {
        for (int j = 0; j < c->width; j++)
        {
            write_block(&c->blocks[i][j], f);
        }
    }
    chunk_free(c);

    fclose(f);
    return 1;
}

int read_chunk(layer_chunk *c, const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return 0;
    if (c != 0)
        chunk_free(c);
    chunk_alloc(c, c->width);

    for (int i = 0; i < c->width; i++)
    {
        for (int j = 0; j < c->width; j++)
        {
            read_block(&c->blocks[i][j], f);
        }
    }
    fclose(f);
    return 1;
}

void make_full_chunk_path(char string[64], world *w, char letter, int x, int y)
{
    sprintf(string, "world_%s/layer_%c/chunk_%d_%d.bin", w->worldname, letter, x, y);
}

int chunk_save(world *w, char letter, int x, int y, layer_chunk *c)
{
    char filename[64];
    make_full_chunk_path(filename, w, letter, x, y);
    return write_chunk(c, filename);
}

int chunk_load(world *w, char letter, int x, int y, layer_chunk *c)
{
    char filename[64];
    make_full_chunk_path(filename, w, letter, x, y);
    return read_chunk(c, filename);
}

void save_layer(world *w, char letter, world_layer *wl)
{
    for (int i = 0; i < wl->size_x; i++)
    {
        for (int j = 0; j < wl->size_y; j++)
        {
            if (wl->chunks[i][j])
                chunk_save(w, letter, i, j, wl->chunks[i][j]);
        }
    }
}

void load_layer(world *w, char letter, world_layer *wl)
{
    for (int i = 0; i < wl->size_x; i++)
    {
        for (int j = 0; j < wl->size_y; j++)
        {
            if (!wl->chunks[i][j])
                chunk_load(w, letter, i, j, wl->chunks[i][j]);
        }
    }
}
