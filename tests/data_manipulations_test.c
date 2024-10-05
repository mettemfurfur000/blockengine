#include "../src/include/data_manipulations.h"
#include "test_utils.h"
#include <stdio.h>

int test_basic_data_manip()
{
	int status = SUCCESS;

	block t = {0, 0};

	int src = 55555;
	int dest = 0;

	block_init(&t, 16, 0, 0);

	status &= data_create_element(&t.data, 't', sizeof(src));
	status &= data_set_i(&t, 't', src);
	status &= data_get_i(&t, 't', &dest);

	status &= (src == dest);

	block_data_free(&t);

	return status;
}

int test_array_data()
{
	int status = SUCCESS;

	block t = {0, 0};

	char cool_string[] = "wholter put yure d away waltr...";
	char not_cool_string[100] = "aah?";

	block_init(&t, 16, 0, 0);

	status &= data_set_str(&t, 's', (byte *)cool_string, strlen(cool_string));
	status &= data_get_str(&t, 's', (byte *)not_cool_string, sizeof(not_cool_string));

	status &= (strcmp(cool_string, not_cool_string) == 0);

	block_data_free(&t);

	return status;
}

void fillrand(char *b, int size)
{
	for (int i = 0; i < size; i++)
		b[i] = 'a' + rand() % 26;
}

void dbg_data_layout(byte *blob)
{
	if (!blob)
	{
		printf("no blob\n");
		return;
	}

	if (blob[0] == 0)
	{
		printf("empty blob\n");
		return;
	}

	short data_size = blob[0] + 1;
	short index = 1;

	printf("data_size %d\n", data_size);

	printf("[\n");
	while (index < data_size)
	{
		byte letter = blob[index];
		byte size = blob[index + 1];
		printf("\t%x:%d:", letter, size);
		for (int i = 0; i < size; i++)
			printf("%02x", blob[index + 2 + i]);
		printf("\n");

		index += blob[index + 1] + 2;
	}
	printf("]\n");
}

int test_random_data()
{
	int status = SUCCESS;

	const block void_block = {0, 0};

	block t = void_block;

	char rand_src[64] = {};
	char rand_dest[64] = {};

	block_init(&t, 16, 0, 0);

	for (int i = 0; i < 1000; i++)
	{
		int rand_size = 1 + rand() % 50;

		fillrand(rand_src, rand_size);

		rand_src[rand_size] = 0;

		// printf("filing %d bytes, %s\n", rand_size, rand_src);

		status &= data_set_str(&t, 's', (byte *)rand_src, rand_size);
		status &= data_get_str(&t, 's', (byte *)rand_dest, rand_size);

		if (strcmp(rand_src, rand_dest) == 0)
		{
			status &= 1;
		}
		else
		{
			printf("data not matching: %s\n%s\n", rand_src, rand_dest);
			status &= 0;
		}
	}

	block_data_free(&t);

	return status;
}

int test_full_test()
{
	int status = SUCCESS;

	block t = {0, 0};
	block_init(&t, 16, 0, 0);

	int runs = 100;
	for (int i = 0; i < runs; i++)
	{
		long long number_in = rand() % 100;
		long long number_out = 0;
		// dbg_data_layout(t.data);
		status &= data_set_number(&t, 't', number_in);
		// dbg_data_layout(t.data);
		status &= data_get_number(&t, 't', (long long *)&number_out);
		// dbg_data_layout(t.data);

		status &= (number_in == number_out);
		if (number_in != number_out)
		{
			printf("number mismath: %lld != %lld\n", number_in, number_out);
		}
	}

	block_data_free(&t);

	return status;
}

int test_all_data_manip()
{
	INIT_TESTING()
	printf("test_all_data_manip:\n");
	RUN_TEST(test_basic_data_manip)
	RUN_TEST(test_array_data)
	RUN_TEST(test_random_data)
	RUN_TEST(test_full_test)

	FINISH_TESTING()
}