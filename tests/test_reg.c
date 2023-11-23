#include "../src/block_registry.c"
#include "utils.h"

int test_read_from_string()
{
	int status = SUCCESS;
	byte *data_out;
	char *test_string = "{\nbyte a = 30\nint i = 1024\ndigit d = 666\nstring @ Hello gordon freeman!\n}";
	byte data_must_be[] = {36, // size byte
						   'a', 1, 0x1e,
						   'i', 4, 0x00, 0x04, 0x00, 0x00,
						   'd', 2, 0x9a, 0x02,					  // digit is folded to minimal possible size
						   '@', 21, 'H', 'e', 'l', 'l', 'o', ' ', // strings is just strings
						   'g', 'o', 'r', 'd', 'o', 'n', ' ',
						   'f', 'r', 'e', 'e', 'm', 'a', 'n', '!'};

	// actually my cool data type is just pascal strings, sad.

	status = make_block_data_from_string(test_string, &data_out);

	status &= !strncmp((const char *)data_out, (const char *)data_must_be, data_must_be[0]);

	return status;
}

int test_parse_from_file()
{
	int status = SUCCESS;
	block_resources br = {0};

	status = parse_block_resources_from_file("resources/blocks/test.blk", &br);

	if (!status)
		return FAIL;

	printf("\t\tblock: %d, %.*s\n", br.block_sample.id,
		   br.block_sample.data[0],
		   br.block_sample.data + 1);

	free_block_resources(&br);

	return status;
}

int test_parse_folder()
{
	int status = SUCCESS;
	block_registry_t b_reg;
	vec_init(&b_reg);

	status = read_block_registry("resources/blocks", &b_reg);

	for (int i = 0; i < b_reg.length; i++)
	{
		printf("\t\tblock: %d,", b_reg.data[i].block_sample.id);

		b_reg.data[i].block_sample.data ? printf("%.*s\n", b_reg.data[i].block_sample.data[0], b_reg.data[i].block_sample.data + 1)
										: printf("(no data)\n");
	}

	free_block_registry(&b_reg);

	return status;
}

int test_all_registry()
{
	init_graphics();
	printf("test_all_registry:\n");
	RUN_TEST(test_read_from_string)
	RUN_TEST(test_parse_from_file)
	RUN_TEST(test_parse_folder)

	exit_graphics();
	return SUCCESS;
}