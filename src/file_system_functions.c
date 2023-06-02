#ifndef FILE_SYSTEM_FUNCTIONS_H
#define FILE_SYSTEM_FUNCTIONS_H 1

#include "memory_control_functions.c"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

const int bigendianchecker = 1;
bool is_bigendian() { return (*(char *)&bigendianchecker) == 0; };

struct stat st = {0};

void make_full_chunk_path(char filename[256], world *w, int index, int x, int y)
{
    char working_path[256];
    getcwd(working_path, 256);
    sprintf(filename, "%s/world_%s", working_path, w->worldname);

    if (stat(filename, &st) == -1)
        mkdir(filename, 0700);

    sprintf(filename, "%s/layer_%d", filename, index);

    if (stat(filename, &st) == -1)
        mkdir(filename, 0700);

    sprintf(filename, "%s/chunk_%d_%d.bin", filename, x, y);
}

void make_full_layer_path(char filename[256], world *w, int index)
{
    char working_path[256];
    getcwd(working_path, 256);

    sprintf(filename, "%s/world_%s", working_path, w->worldname);

    if (stat(filename, &st) == -1)
        mkdir(filename, 0700);

    sprintf(filename, "%s/layer_%d", filename, index);

    if (stat(filename, &st) == -1)
        mkdir(filename, 0700);

    sprintf(filename, "%s/layer_info.bin", filename);
}

void make_full_world_path(char filename[256], world *w, char world_name[64])
{
    char working_path[256];
    getcwd(working_path, 256);

    sprintf(filename, "%s/world_%s", working_path, w->worldname);

    if (stat(filename, &st) == -1)
        mkdir(filename, 0700);

    sprintf(filename, "%s/world_info.bin", filename);
}
/* write/read functions */
void write_block(block *b, FILE *f)
{
    int size = 1;
    fwrite(&b->id, sizeof(int), 1, f);
    if (b->data)
    {
        size = b->data[0] + 1;
        fwrite(b->data, sizeof(byte), size, f);
        return;
    }
    fputc(0,f);
}

void read_block(block *b, FILE *f)
{
    block_data_free(b);

    fread(&b->id, sizeof(int), 1, f);

    byte size = 0;
    fread(&size, sizeof(byte), 1, f);

    if(size)
    {
        b->data = (byte *)calloc(size + 1, sizeof(byte));
        b->data[0] = size;
        fread(b->data + 1, sizeof(byte), size, f);
        return;
    }
}

int write_chunk(layer_chunk *c, const char *filename)
{
    if (!c)
        return FAIL;

    FILE *f = fopen(filename, "wb");
    if (!f)
        return FAIL;

    fwrite(&c->width, sizeof(int), 1, f);

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
    if (!c)
        return FAIL;

    FILE *f = fopen(filename, "rb");

    if (!f)
        return FAIL;

    chunk_free(c);

    fread(&c->width, sizeof(int), 1, f);

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

int write_layer(world_layer *wl, const char *filename)
{
    if (!wl)
        return FAIL;
    FILE *f = fopen(filename, "wb");
    if (!f)
        return FAIL;

    fwrite(&wl->index, sizeof(char), 1, f);
    fwrite(&wl->chunk_width, sizeof(int), 1, f);
    fwrite(&wl->size_x, sizeof(int), 1, f);
    fwrite(&wl->size_y, sizeof(int), 1, f);

    fclose(f);
    return SUCCESS;
}

int read_layer(world_layer *wl, const char *filename)
{
    if (!wl)
        return FAIL;

    FILE *f = fopen(filename, "rb");
    if (!f)
        return FAIL;

    fread(&wl->index, sizeof(char), 1, f);
    fread(&wl->chunk_width, sizeof(int), 1, f);
    fread(&wl->size_x, sizeof(int), 1, f);
    fread(&wl->size_y, sizeof(int), 1, f);

    fclose(f);
    return SUCCESS;
}

int write_world(world *w, const char *filename)
{
    if (!w)
        return FAIL;

    FILE *f = fopen(filename, "wb");
    int errsv = errno;
    if (!f)
        return FAIL;

    fwrite(&w->depth, sizeof(int), 1, f);
    fwrite(&w->worldname, sizeof(char), 64, f);

    fclose(f);
    return SUCCESS;
}

int read_world(world *w, const char *filename)
{
    if (!w)
        return FAIL;

    FILE *f = fopen(filename, "rb");
    if (!f)
        return FAIL;

    fread(&w->depth, sizeof(int), 1, f);
    fread(&w->worldname, sizeof(char), 64, f);

    fclose(f);
    return SUCCESS;
}
/*load/save fucntions*/
// unload means save to file and then free memory

int chunk_save(world *w, int index, int chunk_x, int chunk_y, layer_chunk *c)
{
    char filename[256];
    make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
    return write_chunk(c, filename);
}

int chunk_load(world *w, int index, int chunk_x, int chunk_y, layer_chunk *c)
{
    char filename[256];
    make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
    return read_chunk(c, filename);
}

int chunk_unload(world *w, int index, int chunk_x, int chunk_y, layer_chunk *c)
{
    int ret;
    char filename[256];

    make_full_chunk_path(filename, w, index, chunk_x, chunk_y);
    ret = write_chunk(c, filename);

    if (ret)
        chunk_free(c);
    return ret;
}

void layer_save(world *w, world_layer *wl)
{
    char filename[256];
    make_full_layer_path(filename, w, wl->index);
    write_layer(wl, filename);

    for (int i = 0; i < wl->size_x; i++)
    {
        for (int j = 0; j < wl->size_y; j++)
        {
            if (wl->chunks[i][j])
                chunk_save(w, wl->index, i, j, wl->chunks[i][j]);
        }
    }
}

void layer_load(world *w, world_layer *wl, int index)
{
    char filename[256];
    make_full_layer_path(filename, w, index);
    read_layer(wl, filename);

    world_layer_alloc(wl, wl->size_x, wl->size_y, wl->chunk_width, index);

    for (int i = 0; i < wl->size_x; i++)
    {
        for (int j = 0; j < wl->size_y; j++)
        {
            chunk_load(w, wl->index, i, j, wl->chunks[i][j]);
        }
    }
}

void layer_unload(world *w, world_layer *wl)
{
    char filename[256];
    make_full_layer_path(filename, w, wl->index);
    write_layer(wl, filename);

    for (int i = 0; i < wl->size_x; i++)
    {
        for (int j = 0; j < wl->size_y; j++)
        {
            if (wl->chunks[i][j])
                chunk_unload(w, wl->index, i, j, wl->chunks[i][j]);
        }
    }

    world_layer_free(wl);
}

void world_save(world *w)
{
    char filename[256];
    make_full_world_path(filename, w, w->worldname);
    write_world(w, filename);

    for (int i = 0; i < w->depth; i++)
    {
        layer_save(w, &w->layers[i]);
    }
}

void world_load(world *w, char *worldname)
{
    char filename[256];
    make_full_world_path(filename, w, worldname);
    read_world(w, filename);

    for (int i = 0; i < w->depth; i++)
    {
        layer_load(w, &w->layers[i], i);
    }
}

void world_unload(world *w)
{
    char filename[256];
    make_full_world_path(filename, w, w->worldname);
    write_world(w, filename);

    for (int i = 0; i < w->depth; i++)
    {
        layer_unload(w, &w->layers[i]);
    }
}

#endif