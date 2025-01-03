#include "../include/block_properties.h"
#include "test_utils.h"

int test_basic_io()
{
	int status = 1;
	char *file = "resources/test.prop";

	hash_node **table = alloc_table();

	status &= load_properties(file, table) == SUCCESS;
	status &= save_properties(file, table) == SUCCESS;

	free_table(table);

	return status;
}

INIT_TESTING(test_block_props_all)

RUN_TEST(test_basic_io)

FINISH_TESTING()
