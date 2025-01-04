#include "../include/block_registry.h"
#include "test_utils.h"

int test_read_from_string()
{
	int status = 1;
	blob data_out;

	char *test_string = "{\nbyte a = 30\nint i = 1024\ndigit d = 666\nstring @ Hello gordon freeman!\n}";
	u8 data_must_be[] = {'a', 1, 0x1e,
						 'i', 4, 0x00, 0x04, 0x00, 0x00,
						 'd', 2, 0x9a, 0x02,					// digit is folded to minimal possible size
						 '@', 21, 'H', 'e', 'l', 'l', 'o', ' ', // strings is just strings
						 'g', 'o', 'r', 'd', 'o', 'n', ' ',
						 'f', 'r', 'e', 'e', 'm', 'a', 'n', '!'};
	// 36 bytes

	// actually my cool data type is just pascal strings, sad.

	status &= make_block_data_from_string(test_string, &data_out) == SUCCESS;

	status &= memcmp(data_out.ptr, (const char *)data_must_be, data_out.size) == 0;

	return status;
}

int test_parse_from_file()
{
	int status = 1;
	block_resources br = {};

	status &= parse_block_resources_from_file("resources/blocks/test.blk", &br) == SUCCESS;

	if (!status)
		return FAIL;

	LOG_INFO("\t\tblock: %lld, %.*s", br.id,
			 br.tags.size,
			 br.tags.str);

	print_table(br.all_fields);

	free_block_resources(&br);

	return status;
}

void print_registry(block_registry_t br)
{
	LOG_INFO("printing registry with size %d:\n", br.length);

	for (int i = 0; i < br.length; i++)
	{
		LOG_INFO("\t\tblock: %lld,", br.data[i].id);

		br.data[i].tags.ptr ? LOG_INFO("%.*s\n", br.data[i].tags.size,
									   br.data[i].tags.ptr)
							: LOG_INFO("(no data)\n");
	}
}

int test_parse_folder()
{
	int status = 1;
	block_registry_t b_reg;
	vec_init(&b_reg);

	status = read_block_registry("resources/blocks", &b_reg);

	print_registry(b_reg);

	free_block_registry(&b_reg);

	return status;
}

int test_sort()
{
	int status = 1;
	block_registry_t b_reg;
	vec_init(&b_reg);

	status &= read_block_registry("resources/blocks", &b_reg);

	print_registry(b_reg);

	sort_by_id(&b_reg);

	print_registry(b_reg);

	free_block_registry(&b_reg);

	return status;
}

INIT_TESTING(test_all_registry)

init_graphics();
RUN_TEST(test_read_from_string)
RUN_TEST(test_parse_from_file)
RUN_TEST(test_parse_folder)
RUN_TEST(test_sort);
exit_graphics();

FINISH_TESTING()
