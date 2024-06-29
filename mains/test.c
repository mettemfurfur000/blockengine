#include "../tests/test_mcf.c"
#include "../tests/test_upd.c"
#include "../tests/test_fsf.c"
#include "../tests/test_hst.c"
#include "../tests/test_bps.c"
#include "../tests/test_dmp.c"
#include "../tests/test_reg.c"

#include "../src/include/endianless.h"

// int main(int argc, char* argv[])
// int main()
int main(int argc, char *argv[])
{
	fprintf(stderr, "%s\n", "test thing");
	printf("Big endian? %s\n", is_big_endian() ? "TRUE" : "FALSE");

	int successes = 0;
#define tests 7

	successes += test_block_all();
	successes += test_block_operations_all();
	successes += test_world_all();
	successes += test_hash_table_all();
	successes += test_block_props_all();
	successes += test_all_data_manip();
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

	return 0;
}
