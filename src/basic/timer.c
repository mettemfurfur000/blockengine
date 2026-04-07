#include "include/timer.h"

#include <time.h>

u32 timer_total;
timer_entry timer_list[TIMERS_MAX];

void timer_check_all()
{
	f32 cur_time = time_sec();

	for (u32 i = 0; i < TIMERS_MAX; i++)
	{
		timer_entry e = timer_list[i];

		if (!e.func) // skip empty
			continue;

		if (cur_time > e.time_to_exec)
		{
			e.func(e.data);
			timer_list[i] = (timer_entry){};
		}
	}
}

void timer_add(timer_func f, f32 sec_delay, u64 payload)
{
	f32 cur_time = time_sec();

	for (u32 i = 0; i < TIMERS_MAX; i++)
	{
		timer_entry e = timer_list[i];

		if (e.func) // skip active
			continue;

		e.func = f;
		e.time_to_exec = cur_time + sec_delay;
		e.data = payload;

		timer_list[i] = e;

		return;
	}

	assert(0 && "We outta timers");
}

f32 time_sec();

float time_sec()
{
	return (float)(clock()) / CLOCKS_PER_SEC;
}
