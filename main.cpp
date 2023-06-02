#include "tests/test_mcf.c"
#include "tests/test_fsf.c"
#include "tests/test_hst.c"
#include "tests/test_bps.c"

int main()
{
    test_block_props_all();
    return 0;
}