#define RUN_TEST(f_name)          \
	printf("	%s:", #f_name); \
	printf("\t\t%s\n", f_name() ? "SUCCESS" : "FAIL");
