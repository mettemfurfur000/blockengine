#include "../include/hashtable.h"
#include "../include/benchmarking.h"

#include "test_utils.h"

void blob_generate(blob *b, u32 seed)
{
	SAFE_FREE(b->ptr);

	b->size = seed % 1000;
	b->ptr = malloc(b->size);

	srand(seed);

	for (int i = 0; i < b->size; i++)
		b->ptr[i] = (char)(rand() % 256);
}

int test_rand_fill_and_remove()
{
	hash_node **table = alloc_table();
	int status = 1;

	blob key = {};
	blob val = {};

	blob ret = {};

	for (int i = 1; i < 100; i++)
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

// int test_hash_table_fill()
// {
// 	hash_table **table = alloc_table();
// 	char key[128];
// 	char val[128];
// 	char *ret;
// 	int rand_val;
// 	int status = 1;
// 	int debug = 55;

// 	const int tests = 1000;
// 	double avg_random_get = 0;
// 	float filling_time;

// 	int bench_start_time;

// 	bench_start_time = bench_start();

// 	for (int i = 0; i < tests; i++)
// 	{
// 		rand_val = rand();

// 		blob_generate(key, rand_val);

// 		blob_generate(val, rand_val);

// 		put_entry(table, key, val);
// 	}

// 	filling_time = bench_end(bench_start_time);

// #define RUNS 5
// 	for (int j = 0; j < RUNS; j++)
// 	{
// 		bench_start_time = bench_start();

// 		for (int i = 0; i < tests; i++)
// 		{
// 			rand_val = rand();

// 			blob_generate(key, rand_val);

// 			blob_generate(val, rand_val);

// 			ret = get_entry(table, key);

// 			if (ret)
// 				debug &= (strcmp(val, ret) == 0);
// 		}
// 		avg_random_get += bench_end(bench_start_time);
// 	}
// 	avg_random_get = avg_random_get / RUNS;
// 	int gets_per_second = (1 / avg_random_get) * tests;

// 	free_table(table);

// 	printf("filling time = %f, average get time: %f, gets per second: %d\n", filling_time, avg_random_get, gets_per_second);

// 	return status;
// }

int test_hash_table_all()
{
	INIT_TESTING()
	printf("test_hash_table_all:\n");
	RUN_TEST(test_rand_fill_and_remove)
	// RUN_TEST(test_hash_table_fill)

	FINISH_TESTING()
}