#include "tests/test_mcf.c"
#include "tests/test_fsf.c"
#include "tests/test_hst.c"
#include "tests/test_bps.c"
#include "tests/test_dmp.c"
#include "tests/test_reg.c"

int main()
{
	printf("Big endian? %s\n",is_big_endian() ? "TRUE": "FALSE");
	
    // test_block_all();
    // test_world_all();
    test_hash_table_all();
    test_block_props_all();
    // test_all_data_manip();
	// test_all_registry();
    return 0;
}