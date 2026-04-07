#ifndef TIMER_H
#define TIMER_H

#include "general.h"
#include <assert.h>

typedef void (*timer_func)(u64);

typedef struct
{
	u64 data;
	timer_func func;
	f32 time_to_exec;
	u32 unused;
} timer_entry;

static_assert(sizeof(timer_entry) == 24, "");

#define TIMERS_MAX 256

extern u32 timer_total;
extern timer_entry timer_list[TIMERS_MAX];

void timer_check_all();
void timer_add(timer_func f, f32 sec_delay, u64 payload);

f32 time_sec();

#endif