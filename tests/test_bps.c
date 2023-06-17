#include "../src/block_properties.c"

int test_basic_io()
{
    int status = 1;
    char *file = (char *)"test.prop";
    hash_table **table = alloc_table();

    load_properties(file, table);

    save_properties(file, table);

    free_table(table);

    return status;
}

int test_random_save_cycle()
{
    int status = 1;
    char *file = (char *)"test.prop";
    hash_table **table = alloc_table();

    int tests = 150;

    for (int i = 0; i < tests; i++)
    {
        put_random_entry(table, i + 66);
    }

    for (int i = 0; i < tests; i++)
    {
        status &= verify_random_entry(table, i + 66);
    }

    //print_table(table);

    free_table(table);

    return status;
}

int test_block_props_all()
{
    printf("test_block_props_all:\n");
    printf("    test_basic_io:          %s\n", test_basic_io() ? "SUCCESS" : "FAIL");
    printf("    test_random_save_cycle: %s\n", test_random_save_cycle() ? "SUCCESS" : "FAIL");
    return SUCCESS;
}