#include "include/signal_handler.h"
#include "include/tkv.h"
#include "include/data_io.h"

#include <lua.h>

int test_add(void);
int test_stream_io(void);
int test_binary_serializers(void);

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

	if (strcmp(str_in, "--test-add") == 0)
		return test_add();

	if (strcmp(str_in, "--test-serializers") == 0)
		return test_binary_serializers();

	if (strcmp(str_in, "--test-stream-io") == 0)
		return test_stream_io();

	arena *scratchpad_arena = arena_create(256);
	arena *tkv_arena = arena_create(256);

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

			if(val.meta.tkv_value_type == TKV_VALUE_TKV)
			{
				tkv_object child = tkv_value_to_tkv(val);
				char *serialized = tkv_serialize_object(child, scratchpad_arena);
				printf("'%s' = {\n%s}\n", input, serialized);
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
		scratchpad_arena->length = 0;

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

int test_add(void)
{
	const char *src = "{ i64 existing = 42; }";
	arena *scratch = arena_create(4096);
	arena *tkv_arena = arena_create(4096);

	tkv_object obj = tkv_parse_object(&src, scratch, tkv_arena);
	assert(obj);

	i32 new_val = 100;
	tkv_object extended = tkv_object_add_field(obj, "added_i32", TKV_VALUE_I32, TKV_STATE_VOLATILE, &new_val, tkv_arena);
	assert(extended);

	tkv_value v = tkv_get_value(extended, "added_i32");
	assert(v.meta.whole != UINT_MAX);
	assert(v.meta.tkv_value_type == TKV_VALUE_I32);
	assert(v.meta.tkv_value_state == TKV_STATE_VOLATILE);
	assert(tkv_value_to_i32(v) == 100);

	v = tkv_get_value(extended, "existing");
	assert(v.meta.whole != UINT_MAX);
	assert(tkv_value_to_i64(v) == 42);

	scratch->length = 0;
	char *out = tkv_serialize_object(extended, scratch);
	assert(out);
	printf("--- test_add result ---\n%s\n", out);

	// Add a string field
	const char *hello = "hello world";
	tkv_object with_str = tkv_object_add_field(extended, "greeting", TKV_VALUE_STR, TKV_STATE_CONST, hello, tkv_arena);
	assert(with_str);
	v = tkv_get_value(with_str, "greeting");
	assert(v.meta.whole != UINT_MAX);
	assert(strcmp(tkv_value_to_str(v), "hello world") == 0);

	// Add a bool field
	bool bval = true;
	tkv_object with_bool = tkv_object_add_field(with_str, "flag", TKV_VALUE_BOOL, TKV_STATE_CONST, &bval, tkv_arena);
	assert(with_bool);
	v = tkv_get_value(with_bool, "flag");
	assert(v.meta.whole != UINT_MAX);
	assert(tkv_value_to_bool(v) == true);

	// Add a vec3 field
	vec3 v3 = {1.5f, 2.5f, 3.5f};
	tkv_object with_vec3 = tkv_object_add_field(with_bool, "position", TKV_VALUE_VEC3, TKV_STATE_NETWORKABLE, &v3, tkv_arena);
	assert(with_vec3);
	v = tkv_get_value(with_vec3, "position");
	assert(v.meta.whole != UINT_MAX);
	vec3 read_v3 = tkv_value_to_vec3(v);
	assert(read_v3.x == 1.5f && read_v3.y == 2.5f && read_v3.z == 3.5f);

	// Verify duplicate key rejection
	tkv_object dup = tkv_object_add_field(with_vec3, "flag", TKV_VALUE_BOOL, TKV_STATE_CONST, &bval, tkv_arena);
	assert(dup == NULL);

	// Add a nested TKV field
	const char *child_src = "{ i64 inner = 99; }";
	const char *csp = child_src;
	tkv_object child = tkv_parse_object(&csp, scratch, tkv_arena);
	assert(child);
	tkv_object with_child = tkv_object_add_field(with_vec3, "child", TKV_VALUE_TKV, TKV_STATE_CONST, child, tkv_arena);
	assert(with_child);
	v = tkv_get_value(with_child, "child");
	assert(v.meta.whole != UINT_MAX);
	assert(v.meta.tkv_value_type == TKV_VALUE_TKV);
	tkv_object child_read = tkv_value_to_tkv(v);
	v = tkv_get_value(child_read, "inner");
	assert(tkv_value_to_i64(v) == 99);

	scratch->length = 0;
	out = tkv_serialize_object(with_child, scratch);
	assert(out);
	printf("--- test_add final (with all fields) ---\n%s\n", out);

	arena_destroy(scratch);
	arena_destroy(tkv_arena);
	printf("--- test_add PASSED ---\n");
	return 0;
}

int test_binary_serializers(void)
{
	u8 buf[256];

	// bool
	memset(buf, 0, sizeof(buf));
	u32 n = tkv_write_value_bool(buf, true);
	assert(n == sizeof(bool));
	assert(*(bool *)buf == true);

	// i32
	n = tkv_write_value_i32(buf, -12345);
	assert(n == sizeof(i32));
	assert(*(i32 *)buf == -12345);

	// str
	n = tkv_write_value_str(buf, "test!");
	assert(n == 6);
	assert(strcmp((char *)buf, "test!") == 0);

	// vec3
	vec3 v3 = {1.0f, 2.0f, 3.0f};
	n = tkv_write_value_vec3(buf, v3);
	assert(n == sizeof(vec3));
	assert(((vec3 *)buf)->x == 1.0f);

	// arr
	u8 arr_data[] = {0xAA, 0xBB, 0xCC};
	n = tkv_write_value_arr(buf, 1, 3, arr_data);
	assert(n == 2 * sizeof(u16) + 3);
	assert(*(u16 *)buf == 1);
	assert(*(u16 *)(buf + sizeof(u16)) == 3);
	assert(buf[4] == 0xAA && buf[5] == 0xBB && buf[6] == 0xCC);

	printf("--- test_binary_serializers PASSED ---\n");
	return 0;
}

int test_stream_io(void)
{
	arena *scratch = arena_create(4096);
	arena *tkv_arena = arena_create(4096);

	// Build a TKV object with various types
	tkv_object obj = tkv_object_create_empty(tkv_arena);
	i32 i32_val = -42;
	obj = tkv_object_add_field(obj, "int_val", TKV_VALUE_I32, TKV_STATE_CONST, &i32_val, tkv_arena);
	f64 f64_val = 3.14159;
	obj = tkv_object_add_field(obj, "pi", TKV_VALUE_F64, TKV_STATE_VOLATILE, &f64_val, tkv_arena);
	const char *hello = "hello stream";
	obj = tkv_object_add_field(obj, "greeting", TKV_VALUE_STR, TKV_STATE_CONST, hello, tkv_arena);
	bool b_val = true;
	obj = tkv_object_add_field(obj, "flag", TKV_VALUE_BOOL, TKV_STATE_CONST, &b_val, tkv_arena);
	vec3 v3 = {1.0f, 2.0f, 3.0f};
	obj = tkv_object_add_field(obj, "pos", TKV_VALUE_VEC3, TKV_STATE_NETWORKABLE, &v3, tkv_arena);

	// Add a nested TKV
	tkv_object child = tkv_object_create_empty(tkv_arena);
	i64 inner = 999;
	child = tkv_object_add_field(child, "inner", TKV_VALUE_I64, TKV_STATE_CONST, &inner, tkv_arena);
	obj = tkv_object_add_field(obj, "child", TKV_VALUE_TKV, TKV_STATE_CONST, child, tkv_arena);

	// Write to a buffer stream
	stream_t ws;
	ws.mode = STREAM_BUF;
	ws.handle.raw.bytes = (vec_u8_t){0};
	ws.handle.raw.read_idx = 0;
	vec_init(&ws.handle.raw.bytes);

	assert(tkv_write_to_stream(obj, &ws) == SUCCESS);

	// Read back from the buffer stream
	stream_t rs;
	rs.mode = STREAM_BUF;
	rs.handle.raw.bytes = ws.handle.raw.bytes;
	rs.handle.raw.read_idx = 0;

	tkv_object loaded = tkv_read_from_stream(&rs, scratch, tkv_arena);
	assert(loaded);

	// Verify all fields

	tkv_value v = tkv_get_value(loaded, "int_val");
	assert(v.meta.whole != UINT_MAX);
	assert(tkv_value_to_i32(v) == -42);

	v = tkv_get_value(loaded, "pi");
	assert(v.meta.whole != UINT_MAX);
	assert(v.meta.tkv_value_state == TKV_STATE_VOLATILE);
	f64 pi_read = tkv_value_to_f64(v);
	assert(pi_read > 3.14 && pi_read < 3.15);

	v = tkv_get_value(loaded, "greeting");
	assert(v.meta.whole != UINT_MAX);
	assert(strcmp(tkv_value_to_str(v), "hello stream") == 0);

	v = tkv_get_value(loaded, "flag");
	assert(v.meta.whole != UINT_MAX);
	assert(tkv_value_to_bool(v) == true);

	v = tkv_get_value(loaded, "pos");
	assert(v.meta.whole != UINT_MAX);
	assert(v.meta.tkv_value_state == TKV_STATE_NETWORKABLE);
	vec3 pos_read = tkv_value_to_vec3(v);
	assert(pos_read.x == 1.0f && pos_read.y == 2.0f && pos_read.z == 3.0f);

	v = tkv_get_value(loaded, "child");
	assert(v.meta.whole != UINT_MAX);
	assert(v.meta.tkv_value_type == TKV_VALUE_TKV);
	tkv_object child_read = tkv_value_to_tkv(v);
	v = tkv_get_value(child_read, "inner");
	assert(tkv_value_to_i64(v) == 999);

	// Verify serialized text round-trip
	scratch->length = 0;
	char *text = tkv_serialize_object(loaded, scratch);
	assert(text);
	printf("--- stream I/O round-trip ---\n%s\n", text);

	vec_deinit(&ws.handle.raw.bytes);
	arena_destroy(scratch);
	arena_destroy(tkv_arena);
	printf("--- test_stream_io PASSED ---\n");
	return 0;
}