#include "../src/file_system_functions.c"
#include <unistd.h>

world *tw = (world*)calloc(1,sizeof(world));

char *test_world = "test_world\0";

int test_world_init()
{
    world_alloc(tw, 2, test_world);
    int status = tw->depth == 2 && strcmp(tw->worldname, test_world) == 0;

    world_free(tw);
    return status;
}

int test_layer_init()
{
    world_alloc(tw, 1, test_world);

    world_layer_alloc(&tw->layers[0], 4, 4, 16, 'a');

    int status = tw->layers[0].index == 'a' && tw->layers[0].chunk_width == 16 && (*tw->layers[0].chunks[1][2]).width == 16 && is_block_void(&(*tw->layers[0].chunks[1][2]).blocks[5][8]);

    world_layer_free(&tw->layers[0]);

    world_free(tw);

    return status;
}

int test_save_world()
{
    char filename[64];
    int status = 0;

    world_alloc(tw, 1, test_world);

    world_layer_alloc(&tw->layers[0], 8, 8, 32, 'a');

    world_save(tw);

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            make_full_chunk_path(filename, tw, 'a', i, j);
            status &= (access(filename, F_OK) == 0);
        }
    }

    world_layer_free(&tw->layers[0]);

    world_free(tw);

    return status;
}

int test_load_world()
{
    char filename[64];
    int status = 0;

    world_alloc(tw, 1, test_world);

    world_layer_alloc(&tw->layers[0], 8, 8, 32, 'a');

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

int test_world_all()
{
    printf("tests:\n");
    printf("    test_world_init: %s\n", test_world_init() ? "SUCCESS" : "FAIL");
    printf("    test_layer_init: %s\n", test_layer_init() ? "SUCCESS" : "FAIL");
    printf("    test_save_world: %s\n", test_save_world() ? "SUCCESS" : "FAIL");
    printf("    test_load_world: %s\n", test_load_world() ? "SUCCESS" : "FAIL");
    return SUCCESS;
}
