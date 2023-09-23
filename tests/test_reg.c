#include "../src/block_registry.c"

int test_read_from_file()
{
	int status = SUCCESS;
	byte *data_out;
	char *test_string = "#START\nbyte a = 30\nint i = 1024\ndigit d = 666\nstring @ Hello gordon freeman!\n#END";
	status = make_block_data_from_string(test_string, &data_out);

	int length = data_out[0] + 1;
	printf("Data length %d, data itself:\n", length);
	for (int i = 0; i < length; i++)
	{
		printf("%d : %.2x %c\n", i,data_out[i], data_out[i]);
	}
	return status;
}

int test_all_registry()
{
	printf("test_all_registry:\n");
	printf("    test_read_from_file:        %s\n", test_read_from_file() ? "SUCCESS" : "FAIL");
	return SUCCESS;
}