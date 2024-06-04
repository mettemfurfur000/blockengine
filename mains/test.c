#include "../tests/test_mcf.c"
#include "../tests/test_fsf.c"
#include "../tests/test_hst.c"
#include "../tests/test_bps.c"
#include "../tests/test_dmp.c"
#include "../tests/test_reg.c"
#include "../tests/test_upd.c"

// int main(int argc, char* argv[])
// int main()
int main(int argc, char *argv[])
{
	fprintf(stderr, "%s\n", "test thing");
	printf("Big endian? %s\n", is_big_endian() ? "TRUE" : "FALSE");

	test_block_all();
	test_block_updates_all();
	test_world_all();
	test_hash_table_all();
	test_block_props_all();
	test_all_data_manip();
	test_all_registry();
	return 0;
}
