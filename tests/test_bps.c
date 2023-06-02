#include "src/block_properties.c"

int test_basic()
{
    int status = 1;
    char* file = (char*)"test.prop";
    hash_table** table = alloc_table();

    load_properties(file,table);

    save_properties(file,table);

    free_table(table);

    return status;
}