#ifndef BLOCK_REGISTRY_H
#define BLOCK_REGISTRY_H 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sdl2_basics.c"
#include "block_properties.c"
#include "endianless.c"
#include "game_types.h"
#include "../vec/src/vec.h"

// new type for vector of textures
typedef vec_t(texture) texture_vec_t;

typedef struct block_resources
{
	block block_sample;

	texture_vec_t textures;
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
		return sizeof(byte);
	}
}

// format:

// <character> <type> = <value>
//  ...
// <character> <type> = <value>

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

	// printf("Lets start! Out string is:\n[%s]\n", str);

	char *token = strtok(str, " \n"); // consume start
	if (strcmp(token, "#START") != 0)
	{
		// printf("Error, wrong start token!\n");
		goto block_data_exit;
	}

	// printf("Got %s token\n", token);

	while (1)
	{
		token = strtok(NULL, " \n"); // type, or end token..

		if (!token)
			goto block_data_exit;

		if (strcmp(token, "#END") == 0)
		{
			// printf("Got %s token\n", token);
			goto block_data_exit;
		}

		int type = str_to_enum(token);
		// printf("Got '%s' token for type\n", token);

		token = strtok(NULL, " \n"); // character

		if (!token)
			goto block_data_exit;

		character = token[0]; // copy just 1 character
		// printf("Got '%c' token for character\n", character);

		memset(data_buffer, 0, 256);
		value = 0;
		value_length = 0;

		// special cases for strings or unknown types
		if (type == STRING || type == UNKNOWN)
		{
			token = strtok_take_all_line();
			// memcpy(data_buffer, token, value_length + 1);
			strcpy((char *)data_buffer, token);
		}
		else
		{
			token = strtok(NULL, " =\n"); // value

			if (!token)
				goto block_data_exit;

			// printf("Got '%s' token for value\n", token);
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

		// printf("Result:\n");

		// for (int i = 0; i < value_length; i++)
		// 	printf("%c", data_buffer[i]);

		// printf("\nor, as data:\n");
		// for (int i = 0; i < value_length; i++)
		// 	printf("%.2x ", data_buffer[i]);

		// printf("\n");

		if (data_iterator > 255)
			goto block_data_exit;
	}
block_data_exit:
	// printf("Now we exiting\n");
	data_to_load[0] = data_iterator - 1; // writing size byte

	*out_data_ptr = (byte *)malloc(data_iterator);

	memcpy(*out_data_ptr, data_to_load, data_iterator);

	free(str);

	return SUCCESS;
}

int parse_block_from_file(char *file_path, block *dest)
{
	hash_table **ht = alloc_table();

	if (!load_properties(file_path, ht))
		return FAIL;

	char id_str[64] = {0};
	strcpy(id_str, get_entry(ht, "id"));

	char data_str[512] = {0};
	strcpy(data_str, get_entry(ht, "data"));

	free_table(ht);

	dest->id = atoi(id_str);

	make_block_data_from_string(data_str, &dest->data);

	return SUCCESS;
}

#endif
