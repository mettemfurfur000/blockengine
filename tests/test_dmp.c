#include "../src/data_manipulations.c"
#include "utils.h"

int test_basic_data_manip()
{
	int status = SUCCESS;

	block t = void_block;

	int src = 55555;
	int dest = 0;

	block_init(&t, 16, 0, 0);

	status &= data_create_element(&t.data, 't', sizeof(src));
	status &= data_set_i(t.data, 't', src);
	status &= data_get_i(t.data, 't', &dest);

	status &= (src == dest);

	block_data_free(&t);

	return status;
}

int test_array_data()
{
	int status = SUCCESS;

	block t = void_block;

	char cool_string[66] = "wholter put yure d away waltr...";
	char not_cool_string[66] = "aah?";

	block_init(&t, 16, 0, 0);

	status &= data_create_element(&t.data, 's', sizeof(cool_string));
	status &= data_set_str(t.data, 's', (byte *)cool_string, sizeof(cool_string));
	status &= data_get_str(t.data, 's', (byte *)not_cool_string, sizeof(not_cool_string));
	status &= (strcmp(cool_string, not_cool_string) == 0);

	block_data_free(&t);

	return status;
}

void fillrand(char *b, int size)
{
	for (int i = 0; i < size; i++)
		b[i] = 'a' + rand() % 26;
}

int test_random_data()
{
	int status = SUCCESS;

	block t = void_block;

	char *rand_src;
	char *rand_dest;

	block_init(&t, 16, 0, 0);

	for (int i = 0; i < 1000; i++)
	{
		int rand_size = 1 + rand() % 50;
		rand_src = (char *)calloc(rand_size, 1);
		rand_dest = (char *)calloc(rand_size, 1);

		fillrand(rand_src, rand_size);

		rand_src[rand_size - 1] = 0;

		status &= data_create_element(&t.data, 's', rand_size);

		status &= data_set_str(t.data, 's', (byte *)rand_src, rand_size);
		status &= data_get_str(t.data, 's', (byte *)rand_dest, rand_size);

		status &= !strcmp(rand_src, rand_dest);

		status &= data_delete_element(&t.data, 's');

		free(rand_src);
		free(rand_dest);
	}

	block_data_free(&t);

	return status;
}

int test_all_data_manip()
{
	printf("test_all_data_manip:\n");
	RUN_TEST(test_basic_data_manip)
	RUN_TEST(test_array_data)
	RUN_TEST(test_random_data)

	return SUCCESS;
}