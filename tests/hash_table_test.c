#include "../include/hashtable.h"
#include "../include/benchmarking.h"

#include "test_utils.h"

int test_rand_fill_and_remove()
{
	hash_node **table = alloc_table();
	int status = 1;

	blob key = {};
	blob val = {};
	blob ret = {};

	for (int i = 1; i < 2000; i++)
	{
		blob_generate(&key, 5 + i);

		if (rand() % 2)
		{
			blob_generate(&val, (7 + i * 11) % 137);
			put_entry(table, key, val);
			ret = get_entry(table, key);

			status &= (blob_cmp(ret, val) == 0);
		}
		else
		{
			remove_entry(table, key);
			ret = get_entry(table, key);

			status &= !ret.ptr;
		}
	}

	free_table(table);
	return status;
}

int test_hash_table_fill()
{
	hash_node **table = alloc_table();

	blob key = {};
	blob val = {};
	blob ret = {};

	int rand_val;
	int status = 1;
	int debug = 55;

	const int tests = 1000;
	double avg_random_get = 0;
	float filling_time;

	int bench_start_time;

	bench_start_time = bench_start();

	for (int i = 0; i < tests; i++)
	{
		rand_val = rand();

		blob_generate(&key, rand_val);

		blob_generate(&val, rand_val);

		put_entry(table, key, val);
	}

	filling_time = bench_end(bench_start_time);

#define RUNS 10
	for (int j = 0; j < RUNS; j++)
	{
		bench_start_time = bench_start();

		for (int i = 0; i < tests; i++)
		{
			rand_val = rand();

			blob_generate(&key, rand_val);

			blob_generate(&val, rand_val);

			ret = get_entry(table, key);

			if (ret.ptr)
				debug &= (blob_cmp(val, ret) == 0);
		}
		avg_random_get += bench_end(bench_start_time);
	}
	avg_random_get = avg_random_get / RUNS;
	int gets_per_second = (1 / avg_random_get) * tests;

	free_table(table);

	LOG_DEBUG("filling time = %f, average get time: %f, gets per second: %d", filling_time, avg_random_get, gets_per_second);

	return status;
}

INIT_TESTING(test_hash_table_all)

RUN_TEST(test_rand_fill_and_remove)
RUN_TEST(test_hash_table_fill)
RUN_TEST(test_rand_fill_and_remove)
RUN_TEST(test_hash_table_fill)
RUN_TEST(test_rand_fill_and_remove)
RUN_TEST(test_hash_table_fill)

FINISH_TESTING()
