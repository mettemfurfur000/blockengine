#include "../src/hash_table.c"
#include <time.h>

char *mk_test(int iskey, int num)
{
    char buf[128] = {0};

    if (iskey)
    {
        sprintf(buf, "testkey_%d", num);
    }
    else
    {
        sprintf(buf, "%d_aboba_%d", num, rand() % 5555);
    }
    char *str = (char *)calloc(strlen(buf), 1);
    strcpy(str, buf);

    return str;
}

void write_test(char *str, int iskey, int num)
{
    if (iskey)
    {
        sprintf(str, "testkey_%d", num);
    }
    else
    {
        sprintf(str, "%d_aboba_%d", num, rand() % 5555);
    }
}

float startTime;
void bench_start()
{
    startTime = (float)clock() / CLOCKS_PER_SEC;
}

void bench_end()
{
    float endTime = (float)clock() / CLOCKS_PER_SEC;

    float timeElapsed = endTime - startTime;

    printf("%f\n", timeElapsed);
}

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
        if (i % 10000 == 0)
        {
            printf("size of table: %d\n", appr_size_of(table));
        }

        rand_val = rand() % 555;

        write_test(key, 1, rand_val);

        if (rand() % 2 == 0)
        {
            write_test(val, 0, rand_val);

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

int test_hash_table_stress()
{
    hash_table **table = alloc_table();
    char key[128];
    char val[128];
    char *ret;
    int rand_val;
    int status = 1;

    int i = 1;

    while (1)
    {
        i++;
        if (i % 10000 == 0)
        {
            printf("size of table: %d\n", appr_size_of(table));
            sleep(1);
            i = 1;
        }

        rand_val = rand() % 555;

        write_test(key, 1, rand_val);

        if (rand() % 2 == 0)
        {
            write_test(val, 0, rand_val);

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

    const int tests = 10000;

    int i = 1;

    bench_start();

    for (int i = 0; i < tests; i++)
    {
        rand_val = rand();

        write_test(key, 1, rand_val);

        write_test(val, 0, rand_val);

        put_entry(table, key, val);
    }

    bench_end();
    for (int j = 0; j < 25; j++)
    {
        bench_start();

        for (int i = 0; i < tests; i++)
        {
            rand_val = rand();

            write_test(key, 1, rand_val);

            write_test(val, 0, rand_val);

            ret = get_entry(table, key);

            if (ret)
                status &= (strcmp(val, ret) == 0);
        }
        bench_end();
    }

    bench_start();
    free_table(table);
    bench_end();
    return status;
}

int test_hash_table()
{
    printf("tests:\n");
    printf("    test_rand_fill_and_remove:        %s\n", test_rand_fill_and_remove() ? "SUCCESS" : "FAIL");
    return SUCCESS;
}