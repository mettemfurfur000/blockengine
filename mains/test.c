// #include "../tests/block_memory_control_test.c"
// #include "../tests/block_operations_test.c"
// #include "../tests/test_utils.h"
// #include "../tests/world_fs.test.c"

#include "../tests/block_properties_test.c"
#include "../tests/block_registry_test.c"
#include "../tests/vars_test.c"
#include "../tests/hash_table_test.c"

#include "../include/endianless.h"

int main(int argc, char *argv[])
{
	printf("Big endian? %s\n", is_big_endian() ? "TRUE" : "FALSE");

	log_start("test.log");

	LOG_DEBUG("test debug");

	int successes = 0;
#define tests 7

	// successes += test_block_all();
	// successes += test_block_operations_all();
	// successes += test_world_all();
	successes += test_hash_table_all();
	successes += test_block_props_all();
	successes += test_vars_all();
	successes += test_all_registry();

	switch (successes)
	{
	case tests:
		printf("TOTAL SUCCESS\n");
		break;
	case 0:
		printf("TOTAL FAILURE\n");
		break;
	default:
		printf("%d/%d SUCCESS\n", successes, tests);
	}

	log_end();

	return 0;
}
