#include "../include/general.h"

#define INIT_TESTING(f_name)           \
	int f_name()                       \
	{                                  \
		LOG_DEBUG(#f_name " started"); \
		int passed = 0;                \
		int total = 0;                 \
		int result = 0;

#define RUN_TEST(f_name)                \
	total++;                            \
	LOG_DEBUG("Executing %s", #f_name); \
	result = f_name();                  \
	passed += result ? 1 : 0;           \
	LOG_DEBUG("Result: %s ", result ? "SUCCESS" : "FAIL");

#define FINISH_TESTING()    \
	return passed == total; \
	}
