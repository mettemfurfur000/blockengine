#include "../src/block_registry.c"

int test_read_from_string()
{
	int status = SUCCESS;
	byte *data_out;
	char *test_string = "#START\nbyte a = 30\nint i = 1024\ndigit d = 666\nstring @ Hello gordon freeman!\n#END";

	status = make_block_data_from_string(test_string, &data_out);
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
		printf("		info texture %i: %s, %dx%d with %d frames\n", i, t.filename, t.width , t.height, t.frames);
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