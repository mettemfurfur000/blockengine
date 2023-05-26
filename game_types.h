#pragma once

typedef unsigned char byte;

typedef world;

typedef struct
{
    int id;

    int data_size;
    byte* data;

    void (*custom_behaviour_function)(world*,char,int,int);
} block;

block void_block = {0,0,0,0};

typedef struct
{
    int width;        //all chunks are squares
    block** blocks;
} layer_chunk;

typedef struct
{
    char letter;

    int size_x;
    int size_y;

    int chunk_width;
    layer_chunk*** chunks;
} world_layer;

struct world
{
    int depth;
    world_layer* layers;
};
