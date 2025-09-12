#include "include/hashtable.h"
#include "include/benchmarking.h"

#include "test_utils.h"

#define TESTS 100000

int test_rand_fill_and_remove()
{
	hash_node **table = alloc_table();

	blob key = {};
	blob val = {};
	blob ret = {};

	for ( u32 i = 1; i < TESTS; i++)
	{
		blob_generate(&key, 5 + i);

		if (rand() % 2)
		{
			blob_generate(&val, (7 + i * 11) % 137);
			put_entry(table, key, val);
			ret = get_entry(table, key);

			if (blob_cmp(ret, val) != 0)
			{
				LOG_DEBUG("hashtable returned a different value for the same key, %s != %s", ret, val);
				return FAIL;
			}
		}
		else
		{
			remove_entry(table, key);
			ret = get_entry(table, key);

			if (ret.ptr != NULL)
			{
				LOG_DEBUG("hashtable returned a value for a removed key, %s", ret);
				return FAIL;
			}
		}
	}

	free_table(table);
	return SUCCESS;
}

int test_hash_table_fill()
{
	hash_node **table = alloc_table();

	blob key = {};
	blob val = {};
	blob ret = {};

	int rand_val;

	double avg_random_get = 0;
	float filling_time;

	int bench_start_time;

	bench_start_time = bench_start();

	for ( u32 i = 0; i < TESTS; i++)
	{
		rand_val = rand();
		blob_generate(&key, rand_val);
		put_entry(table, key, key);
	}

	filling_time = bench_end(bench_start_time);

#define RUNS 5
	for ( u32 j = 0; j < RUNS; j++)
	{
		bench_start_time = bench_start();

		for ( u32 i = 0; i < TESTS; i++)
		{
			rand_val = rand();

			blob_generate(&key, rand_val);
			blob_generate(&val, rand_val);

			ret = get_entry(table, key);

			if (blob_cmp(val, ret) != 0)
			{
				LOG_DEBUG("hashtable returned a different value for the same key, %s != %s", ret, val);

				free_table(table);

				return FAIL;
			}
		}
		avg_random_get += bench_end(bench_start_time);
	}
	avg_random_get = avg_random_get / RUNS;

	free_table(table);

	LOG_INFO("filling time = %f, average get time: %f, gets per second: %d", filling_time, avg_random_get, (1 / avg_random_get) * TESTS);

	return SUCCESS;
}

INIT_TESTING(test_hash_table_all)

RUN_TEST(test_rand_fill_and_remove)
RUN_TEST(test_hash_table_fill)

FINISH_TESTING()
