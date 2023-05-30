#include "tests/test_mcf.c"
#include "tests/test_fsf.c"
#include "tests/test_hst.c"

int main()
{
    test_hash_table();

    //test_hash_table_stress();
    return 0;
}