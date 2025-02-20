#include "../include/vars.h"
#include "test_utils.h"
#include <stdio.h>

int test_basic_data_manip()
{
	int status = 1;

	int src = 55555;
	int dest = 0;

	blob b = {};

	status &= var_push(&b, 't', sizeof(src)) == SUCCESS;
	status &= var_set_i32(&b, 't', src) == SUCCESS;
	status &= var_get_i32(b, 't', &dest) == SUCCESS;

	status &= (src == dest);

	var_delete_all(&b);

	return status;
}

int test_random_data()
{
	int status = 1;

	blob b = {};

	for (char i = 'A'; i < 'z'; i++)
	{
		u32 src = rand() * sizeof(u16) + rand();
		u32 dest = 0;
		status &= var_push(&b, i, sizeof(src)) == SUCCESS;
		status &= var_set_u32(&b, i, src) == SUCCESS;
		status &= var_get_u32(b, i, &dest) == SUCCESS;

		status &= (src == dest);
	}

	var_delete_all(&b);

	return status;
}

void fillrand(char *b, int size)
{
	for (int i = 0; i < size; i++)
		b[i] = 'a' + rand() % 26;
	b[size] = 0;
}

// void dbg_data_layout(u8 *blob)
// {
// 	if (!blob)
// 	{
// 		printf("no blob\n");
// 		return;
// 	}

// 	if (blob[0] == 0)
// 	{
// 		printf("empty blob\n");
// 		return;
// 	}

// 	short data_size = blob[0] + 1;
// 	short index = 1;

// 	printf("data_size %d\n", data_size);

// 	printf("[\n");
// 	while (index < data_size)
// 	{
// 		byte letter = blob[index];
// 		byte size = blob[index + 1];
// 		printf("\t%x:%d:", letter, size);
// 		for (int i = 0; i < size; i++)
// 			printf("%02x", blob[index + 2 + i]);
// 		printf("\n");

// 		index += blob[index + 1] + 2;
// 	}
// 	printf("]\n");
// }

INIT_TESTING(test_vars_all)

RUN_TEST(test_basic_data_manip)
RUN_TEST(test_random_data)

FINISH_TESTING()
