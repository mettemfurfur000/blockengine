#include "test_utils.h"
#include "include/vars.h"
#include "include/vars_utils.h"

int test_basic_data_manip()
{
    const char *test_vars = "{ u8 a = 66 u16 b = 6666 u32 c = 666666 i64 d = -6 str s = \"test quoted string\" u8 q = 95 }";

    blob b = {};

    CHECK(vars_parse(test_vars, &b) != SUCCESS);
    char buf[1024] = {};
    dbg_data_layout(b, buf);

	LOG_DEBUG("result layout: %s",buf);

    vars_free(&b);

    return SUCCESS;
}

int test_full_crap()
{
    const char *test_vars = "{ u8 a = 255; u16 b = 65535; u32 c = 4294967295; u32 j = 2147483647; i64 d = -999999999999 str s = \"damn what a string\" }";

    blob b = {};

    CHECK(vars_parse(test_vars, &b) != SUCCESS);
    char buf[1024] = {};
    dbg_data_layout(b, buf);

	LOG_DEBUG("result layout: %s",buf);

    vars_free(&b);

    return SUCCESS;
}

INIT_TESTING(test_vars_all)

RUN_TEST(test_basic_data_manip)
RUN_TEST(test_full_crap)

FINISH_TESTING()
