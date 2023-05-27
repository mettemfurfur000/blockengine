#ifndef GAME_TYPES
#define GAME_TYPES 1

typedef unsigned char byte;

#define SUCCESS 1
#define FAIL 0

typedef struct
{
    int id;

    int data_size;
    byte *data;
} block;

block void_block = {0, 0, 0};

typedef struct
{
    byte activity;
    // 0 - inactive, will unload
    // 1 - active, stay
    // 2+ - have constantly active blocks, newer auto-unload

    int width; // all chunks has square form
    block **blocks;
} layer_chunk;

typedef struct
{
    int index;

    int size_x;
    int size_y;

    int chunk_width;
    layer_chunk ***chunks;
} world_layer;

typedef struct
{
    char *worldname;

    int depth;
    world_layer *layers;
} world;

#endif