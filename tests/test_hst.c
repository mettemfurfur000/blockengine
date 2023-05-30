#include "../src/hash_table.c"

char *mk_test(int iskey, int num)
{
    char buf[64] = {0};

    if (iskey)
    {
        sprintf(buf, "key%d", num);
    }
    else
    {
        sprintf(buf, "%d_%d", num, rand() % 100);
    }
    char *str = (char *)calloc(strlen(buf), 1);
    strcpy(str, buf);

    return str;
}

int test_rand_fill_and_remove()
{
    hash_table **table = (hash_table **)calloc(TABLE_SIZE, sizeof(hash_table *));
    char *key;
    char *val;
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

        key = mk_test(1, rand_val);

        if (rand() % 2 == 0)
        {
            val = mk_test(0, rand_val);

            put_entry(table, key, val);

            ret = get_entry(table, key);

            status &= (strcmp(ret, val) == 0);
        }
        else
        {
            remove_entry(table, key);

            ret = get_entry(table, key);

            status &= (ret == 0);

            free(key);
        }
    }

    free_table(table);
    return status;
}

int test_hash_table_stress()
{
    hash_table **table = (hash_table **)calloc(TABLE_SIZE, sizeof(hash_table *));
    char *key;
    char *val;
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

        key = mk_test(1, rand_val);

        if (rand() % 2 == 0)
        {
            val = mk_test(0, rand_val);

            put_entry(table, key, val);

            ret = get_entry(table, key);

            status &= (strcmp(ret, val) == 0);

            free(val);
        }
        else
        {
            remove_entry(table, key);

            ret = get_entry(table, key);

            status &= (ret == 0);
        }
        free(key);
    }

    free_table(table);
    return status;
}

int test_hash_table()
{
    printf("tests:\n");
    printf("    test_rand_fill_and_remove:        %s\n", test_rand_fill_and_remove() ? "SUCCESS" : "FAIL");
    return SUCCESS;
}