#include "memory_control_functions.c"
#include <stdio.h>

const int bigendianchecker = 1;
bool is_bigendian() { return (*(char *)&bigendianchecker) == 0; };

void make_full_chunk_path(char string[64], world *w, char letter, int x, int y)
{
    sprintf(string, "world_%s/layer_%c/chunk_%d_%d.bin", w->worldname, letter, x, y);
}
/* blocks functions*/
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
/* chunk functions */
int write_chunk(layer_chunk *c, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f)
        return FAIL;
    for (int i = 0; i < c->width; i++)
    {
        for (int j = 0; j < c->width; j++)
        {
            write_block(&c->blocks[i][j], f);
        }
    }

    fclose(f);
    return SUCCESS;
}

int read_chunk(layer_chunk *c, const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return FAIL;
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
    return SUCCESS;
}

int chunk_save(world *w, char letter, int chunk_x, int chunk_y, layer_chunk *c)
{
    char filename[64];
    make_full_chunk_path(filename, w, letter, chunk_x, chunk_y);
    return write_chunk(c, filename);
}

int chunk_unload(world *w, char letter, int chunk_x,int chunk_y, layer_chunk *c)
{
    int ret;
    char filename[64];

    make_full_chunk_path(filename, w, letter, chunk_x, chunk_y);
    ret = write_chunk(c, filename);

    if(ret)
        chunk_free(c);
    return ret;
}

int chunk_load(world *w, char letter, int chunk_x, int chunk_y, layer_chunk *c)
{
    char filename[64];
    make_full_chunk_path(filename, w, letter, chunk_x, chunk_y);
    return read_chunk(c, filename);
}
/* layers */
void save_layer(world *w, world_layer *wl)
{
    for (int i = 0; i < wl->size_x; i++)
    {
        for (int j = 0; j < wl->size_y; j++)
        {
            if (wl->chunks[i][j])
                chunk_save(w, wl->letter, i, j, wl->chunks[i][j]);
        }
    }
}

void load_layer(world *w, world_layer *wl)
{
    for (int i = 0; i < wl->size_x; i++)
    {
        for (int j = 0; j < wl->size_y; j++)
        {
            if (!wl->chunks[i][j])
                chunk_load(w, wl->letter, i, j, wl->chunks[i][j]);
        }
    }
}

void unload_layer(world *w, world_layer *wl)
{
    for (int i = 0; i < wl->size_x; i++)
    {
        for (int j = 0; j < wl->size_y; j++)
        {
            if (wl->chunks[i][j])
                chunk_unload(w, wl->letter, i, j, wl->chunks[i][j]);
        }
    }
}
/* world */
void save_world(world* w)
{
    for (int i = 0; i < w->depth; i++)
    {
        save_layer(w,&w->layers[i]);
    }
}

void unload_world(world* w)
{
    for (int i = 0; i < w->depth; i++)
    {
        unload_layer(w,&w->layers[i]);
    }
}
