#include "include/block_registry.h"

#define EVAL_SIZE_MATCH(v, t) v >> (sizeof(t) * 8 - 8)
#define RETURN_MATCHING_SIZE(v, t) \
	if (EVAL_SIZE_MATCH(v, t))     \
		return sizeof(t);
// some kind of runtime type detection... returns amout of bytes, occupied by a number
byte length(long long value)
{
	RETURN_MATCHING_SIZE(value, long long)
	RETURN_MATCHING_SIZE(value, long)
	RETURN_MATCHING_SIZE(value, int)
	RETURN_MATCHING_SIZE(value, short)

	return 1;
}

void strip_digit(byte *dest, long long value, int actual_length)
{
	byte *value_p = (byte *)&value;
	for (int i = 0; i < actual_length; i++)
		dest[i] = value_p[i];
}

int str_to_enum(char *type_str)
{
	if (!strcmp(type_str, "digit"))
		return T_DIGIT;
	if (!strcmp(type_str, "long"))
		return T_LONG;
	if (!strcmp(type_str, "int"))
		return T_INT;
	if (!strcmp(type_str, "short"))
		return T_SHORT;
	if (!strcmp(type_str, "byte"))
		return T_BYTE;
	if (!strcmp(type_str, "string"))
		return T_STRING;
	return T_UNKNOWN;
}

int get_length_to_alloc(long long value, int type)
{
	switch (type)
	{
	case T_DIGIT:
		return length(value);
	case T_LONG:
		return sizeof(long);
	case T_INT:
		return sizeof(int);
	case T_SHORT:
		return sizeof(short);
	case T_BYTE:
		return sizeof(byte);
	default:
		return length(value);
	}
}

// format:

// { <character> <type> = <value>
//   <character> <type> = <value>
//   ...
//   <character> <type> = <value> }

// acceptable types: digit, long, int, short, byte, string
//
// strings ends with \n character, rember
//
// if type donesnt match, value will be copied as strings
//
// digits will be stripped for saving space, keep it in mind
//
// other numerical types will save their length in bytes, even if you pass it with value of 0

int make_block_data_from_string(const char *str_to_cpy, byte **out_data_ptr)
{
	if (!str_to_cpy)
		return FAIL;

	if (!out_data_ptr)
		return FAIL;

	char character;
	byte data_buffer[256] = {0};
	byte data_to_load[256] = {0};

	data_to_load[0] = 0;   // size byte
	int data_iterator = 1; // starts with 1 because of size byte at start

	long long value;
	int value_length;

	char *str = malloc(1 + strlen(str_to_cpy));
	strcpy(str, str_to_cpy);

	char *token = strtok(str, " \n"); // consume start token
	if (strcmp(token, "{") != 0)
		goto block_data_exit;

	while (1)
	{
		token = strtok(NULL, " \n"); // type, or end token..

		if (!token)
			goto block_data_exit;

		if (strcmp(token, "}") == 0)
			goto block_data_exit;

		int type = str_to_enum(token);

		token = strtok(NULL, " \n"); // character

		if (!token)
			goto block_data_exit;

		character = token[0]; // copy just 1 character

		memset(data_buffer, 0, 256);
		value = 0;
		value_length = 0;

		// special cases for strings or unknown types
		if (type == T_STRING || type == T_UNKNOWN)
		{
			token = strtok_take_whole_line();
			value_length = strlen(token);
			memcpy(data_buffer, token, value_length + 1);
		}
		else
		{
			token = strtok(NULL, " =\n"); // value

			if (!token)
				goto block_data_exit;
			value = atoll(token);

			value_length = get_length_to_alloc(value, type);

			make_endianless((byte *)&value, value_length);
			memcpy(data_buffer, &value, value_length);
			strip_digit(data_buffer, value, value_length);
		}

		data_to_load[data_iterator] = character;
		data_iterator++;
		data_to_load[data_iterator] = value_length;
		data_iterator++;
		for (int i = 0; i < value_length; i++, data_iterator++)
			data_to_load[data_iterator] = data_buffer[i];

		if (data_iterator > 255)
			goto block_data_exit;
	}
block_data_exit:
	if (data_iterator == 1) // ok, no data
		return SUCCESS;

	data_to_load[0] = data_iterator - 1; // writing size byte

	*out_data_ptr = (byte *)malloc(data_iterator);

	memcpy(*out_data_ptr, data_to_load, data_iterator);

	free(str);

	return SUCCESS;
}

/*
all block resources handlers must return SUCCESS if they handled data

if data contains a special clean token, handler will try to free/clear data from dest resource

also handlers must return FAIL if they cant handle data or if data is invalid
*/

#define DECLARE_DEFAULT_FLAG_HANDLER(name, flag)                                              \
	int block_res_##name##_handler(const char *data, block_resources *dest)                   \
	{                                                                                         \
		FLAG_CONFIGURE(dest->flags, flag, atoi(data) ? 1 : 0, strcmp(data, clean_token) == 0) \
		return SUCCESS;                                                                       \
	}

#define DECLARE_DEFAULT_CHAR_FIELD_HANDLER(name)                            \
	int block_res_##name##_handler(const char *data, block_resources *dest) \
	{                                                                       \
		if (!data)                                                          \
			return FAIL;                                                    \
		if (strcmp(data, clean_token) == 0)                                 \
		{                                                                   \
			dest->name = 0;                                                 \
			return SUCCESS;                                                 \
		}                                                                   \
		dest->name = data[0];                                               \
		return SUCCESS;                                                     \
	}

const static char clean_token[] = "??clean??";

int block_res_id_handler(const char *data, block_resources *dest)
{
	if (!data)
		return FAIL;

	if (strcmp(data, clean_token) == 0)
	{
		dest->block_sample.id = 0;
		return SUCCESS;
	}

	int id = atoi(data);

	dest->block_sample.id = id;

	return SUCCESS;
}

int block_res_data_handler(const char *data, block_resources *dest)
{
	if (!data)
		return FAIL;

	if (strcmp(data, clean_token) == 0)
	{
		block_data_free(&dest->block_sample);
		return SUCCESS;
	}

	return make_block_data_from_string(data, &dest->block_sample.data);
}

int block_res_texture_handler(const char *data, block_resources *dest)
{
	if (!data)
		return FAIL;

	if (strcmp(data, clean_token) == 0)
	{
		free_texture(&dest->block_texture);
		return SUCCESS;
	}

	char texture_full_path[256];
	snprintf(texture_full_path, sizeof(texture_full_path), "resources/textures/%s", data);

	return texture_load(&dest->block_texture, texture_full_path);
}

int block_res_fps_handler(const char *data, block_resources *dest)
{
	if (!data)
		return FAIL;

	if (strcmp(data, clean_token) == 0)
	{
		dest->frames_per_second = 0;
		return SUCCESS;
	}

	int fps = atoi(data);
	dest->frames_per_second = fps;
	return SUCCESS;
}

DECLARE_DEFAULT_FLAG_HANDLER(ignore_type, B_RES_FLAG_IGNORE_TYPE)
DECLARE_DEFAULT_FLAG_HANDLER(position_based_type, B_RES_FLAG_RANDOM_POS)

// int block_res_anim_controller_handler(const char *data, block_resources *dest)
// {
// 	if (!data)
// 		return FAIL;

// 	if (strcmp(data, clean_token) == 0)
// 	{
// 		dest->anim_controller = 0;
// 		return SUCCESS;
// 	}

// 	dest->anim_controller = data[0];
// 	return SUCCESS;
// }

// int block_res_type_controller_handler(const char *data, block_resources *dest)
// {
// 	if (!data)
// 		return FAIL;

// 	if (strcmp(data, clean_token) == 0)
// 	{
// 		dest->type_controller = 0;
// 		return SUCCESS;
// 	}

// 	dest->type_controller = data[0];
// 	return SUCCESS;
// }
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(anim_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(type_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(flip_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(rotation_controller)

int block_res_lua_script_handler(const char *data, block_resources *dest)
{
	if (!data)
		return FAIL;

	if (strcmp(data, clean_token) == 0)
	{
		free(dest->lua_script_filename);
		return SUCCESS;
	}

	int len = strlen(data);
	dest->lua_script_filename = malloc(len + 1);
	memcpy(dest->lua_script_filename, data, len + 1);

	return SUCCESS;
}

/* end of block resource handlers */

const static resource_entry_handler res_handlers[] = {
	{&block_res_id_handler, "id", REQUIRED},
	{&block_res_texture_handler, "texture", REQUIRED},

	{&block_res_data_handler, "data", NOT_REQUIRED},

	//{&block_res_anim_toggle_handler, "is_animated", NOT_REQUIRED, {}, {"frame_controller"}}, //if fps is defined, it is animated
	{&block_res_fps_handler, "fps", NOT_REQUIRED, {}, {"frame_controller"}},

	{&block_res_type_controller_handler, "type_controller", NOT_REQUIRED, {"data"}, {}},
	{&block_res_anim_controller_handler, "frame_controller", NOT_REQUIRED, {"data"}, {"fps"}}, // directly controls the frame of a texture, so it cant used with fps
	{&block_res_flip_controller_handler, "flip_controller", NOT_REQUIRED, {"data"}, {}},
	{&block_res_rotation_controller_handler, "rotation_controller", NOT_REQUIRED, {"data"}, {}},

	{&block_res_ignore_type_handler, "ignore_type", NOT_REQUIRED, {}, {}},
	{&block_res_position_based_type_handler, "random_type", NOT_REQUIRED, {}, {}},

	{&block_res_lua_script_handler, "script", NOT_REQUIRED, {}, {}}

};

const int TOTAL_HANDLERS = sizeof(res_handlers) / sizeof(*res_handlers);

int parse_block_resources_from_file(char *file_path, block_resources *dest)
{
	hash_table **properties = alloc_table();
	int status = SUCCESS;

	if (!load_properties(file_path, properties))
	{
		printf("Error loading block resources: %s\n", file_path);
		status = FAIL;
		free_table(properties);
		return status;
	}

	put_entry(properties, "source_filename", file_path);

	// also include a full file in block resources, for whatever reason
	dest->all_fields = properties;

	char *entry = 0;

	vec_str_t seen_entries;
	vec_init(&seen_entries);

	for (int i = 0; i < TOTAL_HANDLERS; i++)
	{
		entry = get_entry(properties, res_handlers[i].name);

		if (!entry)
		{
			if (res_handlers[i].is_critical)
			{
				printf("Error in \"%s\" : No \"%s\" entry\n", file_path, res_handlers[i].name);
				status = FAIL;
				break;
			}
			continue;
		}
		// check for dependencies
		for (int j = 0; j < sizeof(res_handlers[i].dependencies) / sizeof(void *); j++)
		{
			char *dep = res_handlers[i].dependencies[j];
			if (!dep)
				continue;

			if (seen_entries.length == 0)
			{
				printf("Error in \"%s\": \"%s\" is dependent on \"%s\", but seen entries array is empty\n", file_path, res_handlers[i].name, dep);
				status = FAIL;
				break;
			}
			int k = 0;
			vec_find(&seen_entries, dep, k);
			if (k == -1)
			{
				printf("Error in \"%s\": \"%s\" is dependent on \"%s\"\n", file_path, res_handlers[i].name, dep);
				status = FAIL;
				break;
			}

			// printf("Debug message \"%s\": Dependency \"%s\" is found for \"%s\"\n", file_path, dep, res_handlers[i].name);
		}

		// check for incompatibilities
		// basicly same as incompatibilities but instead of requiring certain fields it skips them
		for (int j = 0; j < sizeof(res_handlers[i].incompabilities) / sizeof(void *); j++)
		{
			char *inc = res_handlers[i].incompabilities[j];
			if (!inc)
				continue;

			if (seen_entries.length == 0)
				continue;

			int k = 0;
			vec_find(&seen_entries, inc, k);
			if (k == -1)
				continue;

			printf("Error in \"%s\": \"%s\" is incompatible with \"%s\"\n", file_path, res_handlers[i].name, inc);
			status = FAIL;
			break;
		}

		if (res_handlers[i].function(entry, dest) == FAIL)
		{
			printf("Error in \"%s\": handler \"%s\" failed to process this data: %s\n", file_path, res_handlers[i].name, entry);
			status = FAIL;
			break;
		}
		else
			(void)vec_push(&seen_entries, res_handlers[i].name);
	}

	return status;
}

int is_already_in_registry(block_registry_t *reg, block_resources *br)
{
	for (int i = 0; i < reg->length; i++)
		if (br->block_sample.id == reg->data[i].block_sample.id)
			return SUCCESS;

	return FAIL;
}

int read_block_registry(const char *folder, block_registry_t *reg)
{
	DIR *directory;
	struct dirent *entry;

	directory = opendir(folder);

	if (directory == NULL)
	{
		printf("Unable to open directory: %s\n", folder);
		closedir(directory);
		return FAIL;
	}

	// push a default void block with id 0
	block_resources filler_entry = {.block_sample = {.id = 0, .data = 0}, .type_controller = 0, .frames_per_second = 0, .anim_controller = 0};
	(void)vec_push(reg, filler_entry);

	while ((entry = readdir(directory)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (entry->d_type == DT_REG)
		{
			block_resources br = {.type_controller = 0, .frames_per_second = 0, .anim_controller = 0};
			char file_to_parse[300];
			snprintf(file_to_parse, sizeof(file_to_parse), "%s/%s", folder, entry->d_name);

			if (parse_block_resources_from_file(file_to_parse, &br) == FAIL)
			{
				free_block_resources(&br);
				continue;
			}

			if (is_already_in_registry(reg, &br))
			{
				printf("Found duplicate resource: %s\n", file_to_parse);
				continue;
			}

			(void)vec_push(reg, br);
		}
	}

	FLAG_SET(filler_entry.flags, B_RES_FLAG_IS_FILLER, 1)

	sort_by_id(reg);

	int orig_length = reg->length;

	for (int i = 1; i < orig_length; i++)
	{
		int block_prev_id = reg->data[i - 1].block_sample.id;
		int block_cur_id = reg->data[i].block_sample.id;

		if (block_prev_id + 1 != block_cur_id)
		{
			for (int j = block_prev_id; j < block_cur_id - 1; j++)
			{
				filler_entry.block_sample.id = j;
				printf("adding a filler entry with an id %d\n", j);
				(void)vec_push(reg, filler_entry);
			}
			break;
		}
	}

	sort_by_id(reg);

	closedir(directory);

	return SUCCESS;
}

int __b_cmp(const void *a, const void *b)
{
	return ((block_resources *)a)->block_sample.id > ((block_resources *)b)->block_sample.id;
}

void sort_by_id(block_registry_t *b_reg)
{
	qsort(b_reg->data, b_reg->length, sizeof(*b_reg->data), __b_cmp);
}

void free_block_resources(block_resources *b)
{
	for (int i = 0; i < TOTAL_HANDLERS; i++)
		if (res_handlers[i].function(clean_token, b) == FAIL)
			printf("Error in \"%s\": handler \"%s\" failed to free block resources\n", b->block_sample.data, res_handlers[i].name);

	free_table(b->all_fields);
}

void free_block_registry(block_registry_t *b_reg)
{
	for (int i = 0; i < b_reg->length; i++)
		free_block_resources(&b_reg->data[i]);
	vec_deinit(b_reg);
}
