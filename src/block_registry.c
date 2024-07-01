#include "include/block_registry.h"

int length(long long value)
{
	if (value >> 32)
		return 8;
	if (value >> 16)
		return 4;
	if (value >> 8)
		return 2;
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

int make_block_data_from_string(char *str_to_cpy, byte **out_data_ptr)
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

int block_res_id_handler(char *data, block_resources *dest)
{
	int id = atoi(data);
	if (id == 0)
		return FAIL;
	dest->block_sample.id = id;
	return SUCCESS;
}

int block_res_data_handler(char *data, block_resources *dest)
{
	return make_block_data_from_string(data, &dest->block_sample.data);
}

int block_res_texture_handler(char *data, block_resources *dest)
{
	char texture_full_path[256];
	snprintf(texture_full_path, sizeof(texture_full_path), "resources/textures/%s", data);

	return texture_load(&dest->block_texture, texture_full_path);
}

int block_res_anim_toggle_handler(char *data, block_resources *dest)
{
	if (strcmp(data, "true") == 0)
	{
		dest->is_animated = 1;
		return SUCCESS;
	}
	if (strcmp(data, "false") == 0)
	{
		dest->is_animated = 0;
		return SUCCESS;
	}

	return FAIL;
}

int block_res_anim_fps_handler(char *data, block_resources *dest)
{
	int fps = atoi(data);
	if (fps == 0)
	{
		dest->is_animated = 0;
		return SUCCESS;
	}
	dest->frames_per_second = fps;
	return SUCCESS;
}

int block_res_anim_controller_handler(char *data, block_resources *dest)
{
	printf("anim controller: [%s]\n", data);
	dest->anim_controller = data[0];
	return SUCCESS;
}

int parse_block_resources_from_file(const char *file_path, block_resources *dest)
{
	hash_table **ht = alloc_table();
	int status = SUCCESS;

	if (!load_properties(file_path, ht))
	{
		printf("Error loading block resources: %s\n", file_path);
		status = FAIL;
		goto emergency_exit;
	}

	char *entry = 0;
	const resource_entry_handler res_handlers[] = {
		{&block_res_id_handler, "id", REQUIRED},
		{&block_res_data_handler, "data", NOT_REQUIRED},
		{&block_res_texture_handler, "texture", REQUIRED},
		{&block_res_anim_toggle_handler, "is_animated", NOT_REQUIRED},
		{&block_res_anim_fps_handler, "fps", NOT_REQUIRED},
		{&block_res_anim_controller_handler, "ctrl_var", NOT_REQUIRED}};

	const int TOTAL_HANDLERS = sizeof(res_handlers) / sizeof(*res_handlers);

	for (int i = 0; i < TOTAL_HANDLERS; i++)
	{
		entry = get_entry(ht, res_handlers[i].name);
		if (entry == 0 && res_handlers[i].is_critical)
		{
			printf("Error: No \"%s\" entry in file %s\n", res_handlers[i].name, file_path);
			status = FAIL;
			break;
		}
		if (entry != 0 && res_handlers[i].function(entry, dest) == FAIL)
		{
			printf("Error occured after calling \"%s\" handler, recieved data: %s\n", res_handlers[i].name, entry);
			status = FAIL;
			break;
		}
	}
emergency_exit:
	free_table(ht);
	return status;
}

void free_block_resources(block_resources *b)
{
	block_data_free(&b->block_sample);
	free_texture(&b->block_texture);
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

	while ((entry = readdir(directory)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (entry->d_type == DT_REG)
		{
			block_resources br = {.is_animated = 0, .frames_per_second = 0, .anim_controller = 0};
			char file_to_parse[300];
			snprintf(file_to_parse, sizeof(file_to_parse), "%s/%s", folder, entry->d_name);
			if (parse_block_resources_from_file(file_to_parse, &br) == FAIL)
				free_block_resources(&br);

			if (is_already_in_registry(reg, &br))
			{
				free_block_resources(&br);
				continue;
			}

			(void)vec_push(reg, br);
		}
	}

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

void free_block_registry(block_registry_t *b_reg)
{
	for (int i = 0; i < b_reg->length; i++)
		free_block_resources(&b_reg->data[i]);
	vec_deinit(b_reg);
}
