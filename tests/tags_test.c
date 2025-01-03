#include "../include/tags.h"
#include "test_utils.h"
#include <stdio.h>

int test_basic_data_manip()
{
	int status = 1;

	int src = 55555;
	int dest = 0;

	blob b = {};

	status &= tag_create(&b, 't', sizeof(src)) == SUCCESS;
	status &= tag_set_i(&b, 't', src, sizeof(src)) == SUCCESS;
	status &= tag_get_i(b, 't', (i64 *)&dest, sizeof(dest)) == SUCCESS;

	status &= (src == dest);

	tag_delete_all(&b);

	return status;
}

int test_array_data()
{
	int status = 1;

	blob b = {};

	char cool_string[] = "wholter put yure d away waltr...";
	char not_cool_string[100] = "aah?";

	status &= tag_set_str(&b, 's', (u8 *)cool_string, strlen(cool_string)) == SUCCESS;
	status &= tag_get_str(b, 's', (u8 *)not_cool_string, sizeof(not_cool_string)) == SUCCESS;

	status &= (strcmp(cool_string, not_cool_string) == 0);

	tag_delete_all(&b);

	return status;
}

void fillrand(char *b, int size)
{
	for (int i = 0; i < size; i++)
		b[i] = 'a' + rand() % 26;
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

int test_random_data()
{
	int status = 1;

	blob b = {};

	char rand_src[64] = {};
	char rand_dest[64] = {};

	for (int i = 0; i < 64; i++)
	{
		int rand_size = 1 + rand() % 50;

		fillrand(rand_src, rand_size);

		rand_src[rand_size] = 0;

		// printf("filing %d bytes, %s\n", rand_size, rand_src);

		status &= tag_set_str(&b, 's', (u8 *)rand_src, rand_size) == SUCCESS;
		if (!status)
		{
			printf("failed to set a string\n");
			break;
		}
		status &= tag_get_str(b, 's', (u8 *)rand_dest, rand_size) == SUCCESS;
		if (!status)
		{
			printf("failed to get a string\n");
			break;
		}

		if (strcmp(rand_src, rand_dest) == 0)
		{
			status &= 1;
		}
		else
		{
			printf("data not matching:\nin:\t%s\nout:\t%s\n", rand_src, rand_dest);
			status &= 0;
		}
	}

	tag_delete_all(&b);

	return status;
}

int test_full_test()
{
	int status = 1;

	blob b = {};

	int runs = 64;
	for (int i = 0; i < runs; i++)
	{
		u32 number_in = rand() + (rand() * 32768);
		u32 number_out = 0;

		// dbg_data_layout(t.data);
		status &= tag_set_i(&b, 't', number_in, sizeof(number_in)) == SUCCESS;
		// dbg_data_layout(t.data);
		status &= tag_get_i(b, 't', (long long *)&number_out, sizeof(number_out)) == SUCCESS;
		// dbg_data_layout(t.data);

		status &= (number_in == number_out);
		if (number_in != number_out)
		{
			printf("number mismath: %d != %d\n", number_in, number_out);
		}
	}

	tag_delete_all(&b);

	return status;
}

INIT_TESTING(test_tags_all)

RUN_TEST(test_basic_data_manip)
RUN_TEST(test_array_data)
RUN_TEST(test_random_data)
RUN_TEST(test_full_test)

FINISH_TESTING()
