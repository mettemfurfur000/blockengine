#include "memory_control_functions.c"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const int bigendianchecker = 1;
bool is_bigendian() { return (*(char *)&bigendianchecker) == 0; };

void write_block(block *b, FILE *f)
{
    fwrite(&b->id, 4, 1, f);
    fwrite(&b->data_size, 4, 1, f);
    fwrite(b->data, 1, b->data_size, f);
}

void read_block(block *b, FILE *f)
{
    fread(&b->id, 4, 1, f);
    fread(&b->data_size, 4, 1, f);
    b->data = (byte *)calloc(b->data_size, 1);
    fread(b->data, 1, b->data_size, f);
}

int save_chunk(layer_chunk *c, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f)
        return 0;
    for (size_t i = 0; i < c->width; i++)
    {
        for (size_t j = 0; j < c->width; j++)
        {
            write_block(&c->blocks[i][j], f);
        }
    }
    fclose(f);
    return 1;
}

int load_chunk(layer_chunk *c, const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return 0;
    for (size_t i = 0; i < c->width; i++)
    {
        for (size_t j = 0; j < c->width; j++)
        {
            read_block(&c->blocks[i][j], f);
        }
    }
    fclose(f);
    return 1;
}

void make_chunk_name(char string[64], char letter, int x, int y)
{
    sprintf(string, "chunk_%c_%d_%d.bin", letter, x, y);
}