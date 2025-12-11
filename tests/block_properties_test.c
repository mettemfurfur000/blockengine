#include "include/block_properties.h"
#include "include/folder_structure.h"
#include "test_utils.h"

int test_basic_io()
{
    char *file = FOLDER_REG SEPARATOR_STR TEST_REGISTRY SEPARATOR_STR "test.prop";

    hash_node **table = alloc_table();

    assert(load_properties(file, table));
    assert(save_properties(file, table));

    free_table(table);

    return SUCCESS;
}

INIT_TESTING(test_block_props_all)

RUN_TEST(test_basic_io)

FINISH_TESTING()
