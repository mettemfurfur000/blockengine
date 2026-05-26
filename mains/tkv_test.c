#include "include/signal_handler.h"
#include "include/tkv.h"
#include "include/tokenizer.h"

#include <lua.h>

int main(int argc, char *argv[])
{
	init_backtrace();
	init_signal_handlers();

	log_start("tkv.log");

	assert(argc >= 2);

	const char *str_in = argv[1];

	bool interactive = false;

	if (strcmp(str_in, "--interactive") == 0)
	{
		assert(argc >= 3);
		str_in = argv[2];
		interactive = true;
	}

	arena *scratchpad_arena = arena_create(1024 * 1024);
	arena *tkv_arena = arena_create(1024 * 1024);

	fflush(stdout);

	const char *src = str_in;
	tkv_object parsed = tkv_parse_object(&src, scratchpad_arena, tkv_arena);

	if (!parsed)
	{
		printf("Failed to parse TKV input\n");
		return 1;
	}

	if (interactive)
	{
		// Read keys, print values until "exit" is entered

		char input[256];

		printf("Enter key to query (or 'exit' to quit)\n");
		while (true)
		{
			printf("> ");
			if (!fgets(input, sizeof(input), stdin))
				break;

			// Remove trailing newline
			input[strcspn(input, "\n")] = 0;

			if (strcmp(input, "exit") == 0)
				break;

			tkv_value val = tkv_traverse_get_value(parsed, input);

			if (val.meta.whole == UINT_MAX || val.ptr == NULL)
			{
				printf("Key '%s' not found\n", input);
				continue;
			}

			char serialized[256];
			tkv_serialize_value((u8 *)serialized, sizeof(serialized), val);
			printf("'%s' = %s\n", input, serialized);
		}

		return 0;
	}

	if (parsed)
	{
		scratchpad_arena->length = 0; // reset scratchpad arena for serialization use

		char *serialized = tkv_serialize_object(parsed, scratchpad_arena);
		if (serialized)
		{
			printf("\n--- Serialized TKV Output ---\n");
			printf("%s\n", serialized);
		}

		return 0;
	}

	return 0;
}