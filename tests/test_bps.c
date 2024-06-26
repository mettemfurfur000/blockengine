#include "../src/include/block_properties.h"
#include "utils.h"

int test_basic_io()
{
	int status = SUCCESS;
	char *file = "resources/test.prop";
	hash_table **table = alloc_table();

	status &= load_properties(file, table);

	status &= save_properties(file, table);

	free_table(table);

	return status;
}

int test_random_save_cycle()
{
	int status = SUCCESS;
	hash_table **table = alloc_table();

	int tests = 150;

	for (int i = 0; i < tests; i++)
		put_random_entry(table, i + 66);

	for (int i = 0; i < tests; i++)
		status &= verify_random_entry(table, i + 66);

	free_table(table);

	return status;
}

int test_block_props_all()
{
	printf("test_block_props_all:\n");
	RUN_TEST(test_basic_io)
	RUN_TEST(test_random_save_cycle)

	return SUCCESS;
}