#include "../include/general.h"

#define TEST_REGISTRY "test"

#define INIT_TESTING(f_name)                  \
	int f_name()                              \
	{                                         \
		LOG_INFO("TEST " #f_name " STARTED"); \
		int passed = 0;                       \
		int total = 0;                        \
		int result = 0;

#define RUN_TEST(f_name)                 \
	total++;                             \
	LOG_INFO("EXECUTING TEST " #f_name); \
	result = f_name();                   \
	passed += result ? 1 : 0;            \
	LOG_INFO("TEST " #f_name " RESULT: %s ", result ? "SUCCESS" : "FAIL");

#define FINISH_TESTING()    \
	return passed == total; \
	}
