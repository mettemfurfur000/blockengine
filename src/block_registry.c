#ifndef BLOCK_REGISTRY_H
#define BLOCK_REGISTRY_H 1

// vscode itellisense wants this define so bad...
#define _DEFAULT_SOURCE 1

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sdl2_basics.c"
#include "block_properties.c"
#include "endianless.c"
#include "game_types.h"
#include "memory_control_functions.c"
#include "../vec/src/vec.h"

#define MAX_PATH_LENGTH 512

typedef struct block_resources
{
	block block_sample;

	texture block_texture;
} block_resources;

typedef vec_t(block_resources) block_registry_t;

enum block_data_types
{
	UNKNOWN,
	DIGIT,
	LONG,
	INT,
	SHORT,
	BYTE,
	STRING,
};

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
		return DIGIT;
	if (!strcmp(type_str, "long"))
		return LONG;
	if (!strcmp(type_str, "int"))
		return INT;
	if (!strcmp(type_str, "short"))
		return SHORT;
	if (!strcmp(type_str, "byte"))
		return BYTE;
	if (!strcmp(type_str, "string"))
		return STRING;
	return UNKNOWN;
}

int get_length_to_alloc(long long value, int type)
{
	switch (type)
	{
	case DIGIT:
		return length(value);
	case LONG:
		return sizeof(long);
	case INT:
		return sizeof(int);
	case SHORT:
		return sizeof(short);
	case BYTE:
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
		if (type == STRING || type == UNKNOWN)
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
	if(data_iterator == 1) // ok, no data
		return SUCCESS;
	
	data_to_load[0] = data_iterator - 1; // writing size byte

	*out_data_ptr = (byte *)malloc(data_iterator);

	memcpy(*out_data_ptr, data_to_load, data_iterator);

	free(str);

	return SUCCESS;
}

typedef struct
{
	int (*function)(char *, block_resources *);
	char *name;
	byte is_critical;
} resource_entry_handler;

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
#define TOTAL_HANDLERS 3
	const resource_entry_handler res_handlers[TOTAL_HANDLERS] = {
		{&block_res_id_handler, "id", 1},
		{&block_res_data_handler, "data", 0},
		{&block_res_texture_handler, "texture", 1}};

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
#undef TOTAL_HANDLERS

void free_block_resources(block_resources *b)
{
	block_data_free(&b->block_sample);
	free_texture(&b->block_texture);
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
			block_resources br = {};
			char file_to_parse[300];
			snprintf(file_to_parse, sizeof(file_to_parse), "%s/%s", folder, entry->d_name);
			if (parse_block_resources_from_file(file_to_parse, &br) == FAIL)
				free_block_resources(&br);

			(void)vec_push(reg, br);
		}
	}

	closedir(directory);

	return SUCCESS;
}

void free_block_registry(block_registry_t *b_reg)
{
	for (int i = 0; i < b_reg->length; i++)
		free_block_resources(&b_reg->data[i]);
	vec_deinit(b_reg);
}

#endif