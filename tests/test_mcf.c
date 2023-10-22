#include "../src/memory_control_functions.c"
#include <stdio.h>
#include "utils.h"

int test_block_init()
{
	block t1 = void_block;

	block_init(&t1, 10, 32, "example");

	int status = t1.id == 10 && t1.data[0] == 32;

	block_data_free(&t1);

	return status;
}

int test_block_data_resize()
{
	block t1 = void_block;

	block_init(&t1, 10, 32, "example");

	block_data_resize(&t1, 25);

	int status = t1.id == 10 && t1.data[0] == 57;

	block_data_free(&t1);

	return status;
}

int test_block_erase()
{
	block t1 = void_block;

	block_init(&t1, 10, 32, "example");

	block_erase(&t1);

	int status = is_block_void(&t1);

	block_data_free(&t1);

	return status;
}

int test_block_copy()
{
	block t1 = void_block;
	block t2 = void_block;

	block_init(&t1, 10, 32, "abcdetest123\0");

	block_copy(&t2, &t1);

	int status = is_block_equal(&t1, &t2);

	block_data_free(&t1);
	block_data_free(&t2);

	return status;
}

int test_block_teleport()
{
	block t1 = void_block;
	block t2 = void_block;

	block t1_copy = void_block;

	block_init(&t1, 10, 32, "abcdetest123\0");

	block_copy(&t1_copy, &t1);

	block_teleport(&t2, &t1);

	int status = is_block_void(&t1) && is_block_equal(&t2, &t1_copy);

	block_data_free(&t1);
	block_data_free(&t2);
	block_data_free(&t1_copy);

	return status;
}

int test_block_swap()
{
	block t1 = void_block;
	block t2 = void_block;

	block t1_copy = void_block;
	block t2_copy = void_block;

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

	return status;
}

int test_block_all()
{
	printf("test_block_all:\n");
	RUN_TEST(test_block_init)
	RUN_TEST(test_block_data_resize)
	RUN_TEST(test_block_erase)
	RUN_TEST(test_block_copy)
	RUN_TEST(test_block_teleport)
	RUN_TEST(test_block_swap)
	
	return SUCCESS;
}