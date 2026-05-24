#include "include/signal_handler.h"
#include "include/tkv.h"
#include "include/tokenizer.h"

#include <lua.h>

int main(int argc, char *argv[])
{
	init_backtrace();
	init_signal_handlers();

	log_start("tkv.log");
	assert(argc == 2);

	const char *str_in = argv[1];

	arena *scratchpad_arena = arena_create(1024 * 1024);
	arena *tkv_arena = arena_create(1024 * 1024);

	fflush(stdout);

	const char *src = str_in;
	tkv_object parsed = tkv_parse_object(&src, scratchpad_arena, tkv_arena);

	// printf("Taken arena space: scratchpad = %u bytes, tkv = %u bytes\n", scratchpad_arena->length, tkv_arena->length);
	// fflush(stdout);

	if (parsed)
	{
		scratchpad_arena->length = 0; // reset scratchpad arena for serialization use

		char *serialized = tkv_serialize_object(parsed, scratchpad_arena);
		if (serialized)
		{
			printf("\n--- Serialized TKV Output ---\n");
			printf("%s\n", serialized);
		}
	}
	else
	{
		printf("Failed to parse TKV\n");
		// printf("All tokens:\n");
		// token_debug_all(str_in);
	}

	deinit_signal_handlers();
	deinit_backtrace();

	return 0;
}