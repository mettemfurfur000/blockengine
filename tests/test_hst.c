#include "../src/hash_table.c"
#include "../src/benchmarking.c"
#include "utils.h"

int test_rand_fill_and_remove()
{
    hash_table **table = alloc_table();
    char key[128];
    char val[128];
    char *ret;
    int rand_val;
    int status = 1;

    for (int i = 1; i < 100000; i++)
    {
        if (i % 20000 == 0)
            printf("        Size of table: %d\n", actual_size_of_table(table));

        rand_val = rand() % 555;

        fill_test_data(key, 1, rand_val);

        if (rand() % 2 == 0)
        {
            fill_test_data(val, 0, rand_val);

            put_entry(table, key, val);

            ret = get_entry(table, key);

            status &= (strcmp(ret, val) == 0);
        }
        else
        {
            remove_entry(table, key);

            ret = get_entry(table, key);

            status &= (ret == 0);
        }
    }

    free_table(table);
    return status;
}

int test_hash_table_fill()
{
    hash_table **table = alloc_table();
    char key[128];
    char val[128];
    char *ret;
    int rand_val;
    int status = 1;
    int debug = 55;

    const int tests = 1000;

    printf("        Started %d test per run\n",tests);

    int bench_start_time;

    bench_start_time = bench_start();

    for (int i = 0; i < tests; i++)
    {
        rand_val = rand();

        fill_test_data(key, 1, rand_val);

        fill_test_data(val, 0, rand_val);

        put_entry(table, key, val);
    }
    printf("        Time elapsed: %f seconds(filling)\n", bench_end(bench_start_time));

    for (int j = 0; j < 5; j++)
    {
        bench_start_time = bench_start();

        for (int i = 0; i < tests; i++)
        {
            rand_val = rand();

            fill_test_data(key, 1, rand_val);

            fill_test_data(val, 0, rand_val);

            ret = get_entry(table, key);

            if (ret)
                debug &= (strcmp(val, ret) == 0);
        }
        printf("        Time elapsed: %f seconds (random get)\n", bench_end(bench_start_time));
    }

    bench_start_time = bench_start();

    free_table(table);

    printf("        Time elapsed: %f seconds (freeing table)\n", bench_end(bench_start_time));

    return status;
}

int test_hash_table_all()
{
    printf("test_hash_table_all:\n");
	RUN_TEST(test_rand_fill_and_remove)
    RUN_TEST(test_hash_table_fill)
	
    return SUCCESS;
}