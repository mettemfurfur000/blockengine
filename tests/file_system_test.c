#include "../include/file_system.h"
#include "../include/vars.h"
#include "../include/vars_utils.h"
#include "test_utils.h"

int test_vars_io()
{
	const char *test_vars = "{ u8 a = 255; u16 b = 65535; u32 c = 4294967295; u32 j = 2147483647; i64 d = -999999999999 str s = \"damn what a string\" }";

    blob b = {};
    char buf[1024] = {};

    CHECK(vars_parse(test_vars, &b) != SUCCESS);

    dbg_data_layout(b, buf);
    LOG_DEBUG("before write layout: %s", buf);

    FILE *f = fopen("test.vars", "wb");
    blob_vars_write(b, f);
    fclose(f);

	f = fopen("test.vars", "rb");
	blob r = blob_vars_read(f);
	fclose(f);

	memset(buf,0,sizeof(buf));
	dbg_data_layout(r, buf);
    LOG_DEBUG("after read layout: %s", buf);

    vars_free(&b);
    vars_free(&r);

    return SUCCESS;
}

INIT_TESTING(test_file_system)

RUN_TEST(test_vars_io)

FINISH_TESTING()