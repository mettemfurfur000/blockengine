#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "logging.h"
#include "timer.h"

#define TIME_GET_SECONDS_TOOK(func_block, result_comment)                                                              \
	do                                                                                                                 \
	{                                                                                                                  \
		f32 started_sec = time_sec();                                                                                  \
		func_block;                                                                                                    \
		f32 took_sec = time_sec() - started_sec;                                                                       \
		LOG_MESSAGE(result_comment ", took %.2f seconds", took_sec);                                                   \
	} while (0)

#endif