#include "../src/file_system_functions.c"
#include <unistd.h>

char test_world[16] = "test_world\0";

int test_world_init()
{
    world *tw = world_make(2, test_world);

    int status = tw->depth == 2 && strcmp(tw->worldname, test_world) == 0;

    world_free(tw);
    return status;
}

int test_layer_init()
{
    world *tw = world_make(1, test_world);

    world_layer_alloc(&tw->layers[0], 4, 4, 16, 0);

    int status = tw->layers[0].index == 0 && tw->layers[0].chunk_width == 16 && (*tw->layers[0].chunks[1][2]).width == 16 && is_block_void(&(*tw->layers[0].chunks[1][2]).blocks[5][8]);

    world_layer_free(&tw->layers[0]);

    world_free(tw);

    return status;
}

int test_save_world()
{
    char filename[256];
    int status = 1;

    world *tw = world_make(1, test_world);

    world_layer_alloc(&tw->layers[0], 8, 8, 32, 0);

    world_save(tw);

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            make_full_chunk_path(filename, tw, 0, i, j);
            status &= (access(filename, F_OK) == 0);
        }
    }

    world_layer_free(&tw->layers[0]);

    world_free(tw);

    return status;
}

int test_load_world()
{
    int status = 1;

    world *tw = world_make(1, test_world);

    world_layer_alloc(&tw->layers[0], 8, 8, 32, 0);

    world_load(tw, test_world);

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            status &= tw->layers[0].chunks[i][j] != 0;
        }
    }

    world_layer_free(&tw->layers[0]);

    world_free(tw);

    return status;
}

int test_save_load_random()
{
    int status = 1;
    int w_width = 16;
    int ch_width = 64;
    layer_chunk *t = (layer_chunk *)calloc(1, sizeof(layer_chunk));

    char rand_world[16] = "rand_world";

    world *tw = world_make(1, rand_world);

    world_layer_alloc(&tw->layers[0], w_width, w_width, ch_width, 0);

    for (int i = 0; i < w_width; i++)
    {
        for (int j = 0; j < w_width; j++)
        {
            chunk_random_fill(tw->layers[0].chunks[i][j], 1 + j + w_width * i);
        }
    }

    world_save(tw);

    world_layer_free(&tw->layers[0]);

    world_free(tw);

    ///////////////////////////////////////////////

    tw = world_make(1, rand_world);

    world_layer_alloc(&tw->layers[0], w_width, w_width, 32, 0);

    world_load(tw, rand_world);

    chunk_alloc(t, ch_width);

    for (int i = 0; i < w_width; i++)
    {
        for (int j = 0; j < w_width; j++)
        {
            chunk_random_fill(t, 1 + j + w_width * i);
            status &= is_chunk_equal(tw->layers[0].chunks[i][j], t);
        }
    }

    world_layer_free(&tw->layers[0]);

    world_free(tw);

    free(t);

    return status;
}

int test_world_all()
{
    printf("test_world_all:\n");
    printf("    test_world_init:       %s\n", test_world_init() ? "SUCCESS" : "FAIL");
    printf("    test_layer_init:       %s\n", test_layer_init() ? "SUCCESS" : "FAIL");
    printf("    test_save_world:       %s\n", test_save_world() ? "SUCCESS" : "FAIL");
    printf("    test_load_world:       %s\n", test_load_world() ? "SUCCESS" : "FAIL");
    printf("    test_save_load_random: %s\n", test_save_load_random() ? "SUCCESS" : "FAIL");
    return SUCCESS;
}
