#include "../src/include/block_memory_control.h"
#include <stdio.h>
#include "test_utils.h"

int test_block_init()
{
	block t1 = {0, 0};

	block_init(&t1, 10, 32, "example");

	int status = t1.id == 10 && t1.data[0] == 32;

	block_data_free(&t1);

	return SUCCESS;
}

int test_block_data_resize()
{
	block t1 = {0, 0};

	block_init(&t1, 10, 32, "example");

	block_data_resize(&t1, 25);

	int status = t1.id == 10 && t1.data[0] == 57;

	block_data_free(&t1);

	return SUCCESS;
}

int test_block_erase()
{
	block t1 = {0, 0};

	block_init(&t1, 10, 32, "example");

	block_erase(&t1);

	int status = is_block_void(&t1);

	block_data_free(&t1);

	return SUCCESS;
}

int test_block_copy()
{
	block t1 = {0, 0};
	block t2 = {0, 0};

	block_init(&t1, 10, 32, "abcdetest123\0");

	block_copy(&t2, &t1);

	int status = is_block_equal(&t1, &t2);

	block_data_free(&t1);
	block_data_free(&t2);

	return SUCCESS;
}

int test_block_teleport()
{
	block t1 = {0, 0};
	block t2 = {0, 0};

	block t1_copy = {0, 0};

	block_init(&t1, 10, 32, "abcdetest123\0");

	block_copy(&t1_copy, &t1);

	block_teleport(&t2, &t1);

	int status = is_block_void(&t1) && is_block_equal(&t2, &t1_copy);

	block_data_free(&t1);
	block_data_free(&t2);
	block_data_free(&t1_copy);

	return SUCCESS;
}

int test_block_swap()
{
	block t1 = {0, 0};
	block t2 = {0, 0};

	block t1_copy = {0, 0};
	block t2_copy = {0, 0};

	block_init(&t1, 10, 32, "abcdetest123\0");
	block_init(&t2, 25, 44, "ajshgajhsgsaj\0");

	block_copy(&t1_copy, &t1);
	block_copy(&t2_copy, &t2);

	block_swap(&t1, &t2);

	int status = is_block_equal(&t1, &t2_copy) && is_block_equal(&t2, &t1_copy);

	block_data_free(&t1);
	block_data_free(&t2);
	block_data_free(&t1_copy);
	block_data_free(&t2_copy);

	return SUCCESS;
}

int test_block_all()
{
	INIT_TESTING()
	printf("test_block_all:\n");
	RUN_TEST(test_block_init)
	RUN_TEST(test_block_data_resize)
	RUN_TEST(test_block_erase)
	RUN_TEST(test_block_copy)
	RUN_TEST(test_block_teleport)
	RUN_TEST(test_block_swap)

	FINISH_TESTING()
}