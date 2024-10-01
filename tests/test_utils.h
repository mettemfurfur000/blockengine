#define INIT_TESTING() \
	int passed = 0;    \
	int total = 0;     \
	int result = 0;

#define RUN_TEST(f_name)        \
	total++;                    \
	printf("	%s:", #f_name); \
	result = f_name();          \
	passed += result ? 1 : 0;   \
	printf("\t\t%s\n", result ? "SUCCESS" : "FAIL");
#define FINISH_TESTING() return passed == total;
