#include "../src/block_registry.c"

int test_read_from_string()
{
	int status = SUCCESS;
	byte *data_out;
	char *test_string = "{\nbyte a = 30\nint i = 1024\ndigit d = 666\nstring @ Hello gordon freeman!\n}";
	byte data_must_be[] = {36, // size byte
						   'a', 1, 0x1e,
						   'i', 4, 0x00, 0x04, 0x00, 0x00,
						   'd', 2, 0x9a, 0x02,					//digit is folded to minimal possible size
						   '@', 21, 'H', 'e', 'l', 'l', 'o', ' ', // strings is just strings
						   'g', 'o', 'r', 'd', 'o', 'n', ' ',
						   'f', 'r', 'e', 'e', 'm', 'a', 'n', '!'};
	
	//actually my cool data type is just pascal strings, sad.

	status = make_block_data_from_string(test_string, &data_out);

	status &= !strncmp((const char *)data_out, (const char *)data_must_be, data_must_be[0]);

	return status;
}

int test_parse_from_file()
{
	int status = SUCCESS;
	block b = void_block;

	status = parse_block_from_file("blocks/test.blk", &b);

	return status;
}

int test_add_to_vector()
{
	int status = SUCCESS;

	init_graphics();

	texture_vec_t test_textures;
	vec_init(&test_textures);

	status &= load_textures_from_folder("textures", &test_textures, 0);

	for (int i = 0; i < test_textures.length; i++)
	{
		texture t = test_textures.data[i];
		printf("		info texture %i: %s, %dx%d with %d frames\n", i, t.filename, t.width, t.height, t.frames);
	}

	exit_graphics();

	return status;
}

int test_all_registry()
{
	printf("test_all_registry:\n");
	printf("    test_read_from_string:        %s\n", test_read_from_string() ? "SUCCESS" : "FAIL");
	printf("    test_parse_from_file:         %s\n", test_parse_from_file() ? "SUCCESS" : "FAIL");
	printf("	test_add_to_vector:			  %s\n", test_add_to_vector() ? "SUCCESS" : "FAIL");
	return SUCCESS;
}