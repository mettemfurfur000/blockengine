#include "../include/vars.h"
#include "test_utils.h"
#include <stdio.h>
#include <time.h>

int test_basic_data_manip()
{
	int src = 55555;
	int dest = 0;

	blob b = {};

	CHECK(var_push(&b, 't', sizeof(src)));
	CHECK(var_set_i32(&b, 't', src));
	CHECK(var_get_i32(b, 't', &dest));

	CHECK(src != dest);

	char buf[1024] = {};
	dbg_data_layout(b, buf);

	var_delete_all(&b);

	return SUCCESS;
}

int test_random_data()
{
	blob b = {};

	for (char i = 'A'; i < 'Z'; i++)
	{
		u32 src = rand() * sizeof(u16) + rand();
		u32 dest = 0;
		CHECK(var_push(&b, i, sizeof(src)));
		CHECK(var_set_u32(&b, i, src));
		CHECK(var_get_u32(b, i, &dest));

		CHECK(src != dest);
	}

	char buf[1024] = {};
	dbg_data_layout(b, buf);

	var_delete_all(&b);

	return SUCCESS;
}

int test_five_hundred_variables()
{
	i32 seed = time(NULL);

	srand(seed);

	blob b = {};

	u32 src = 0x12345678;

	for (int i = 0; i < 26; i++)
	{
		char letter = 'A' + i;
		CHECK(var_set_u32(&b, letter, src));
	}

	for (int i = 0; i < 26; i++)
	{
		char letter = 'A' + i;
		u32 dest = 0;
		CHECK(var_get_u32(b, letter, &dest));
		if (src != dest)
		{
			printf("%c : src %x != dest %x\n", letter, src, dest);
			return FAIL;
		}
		// CHECK(src != dest);
	}

	char buf[1024] = {};
	dbg_data_layout(b, buf);

	var_delete_all(&b);

	return SUCCESS;
}

void fillrand(char *b, int size)
{
	for (int i = 0; i < size; i++)
		b[i] = 'a' + rand() % 26;
	b[size] = 0;
}

INIT_TESTING(test_vars_all)

RUN_TEST(test_basic_data_manip)
RUN_TEST(test_random_data)
RUN_TEST(test_five_hundred_variables)

FINISH_TESTING()
