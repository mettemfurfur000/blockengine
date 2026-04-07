#include "include/block_properties.h"

#include <stdio.h>
#include <string.h>

char *strtok_take_whole_line()
{
	char *token = strtok(NULL, "\n\r"); // take everything until first newline character
	if (!token)
		return 0;

	while (*token == ' ') // skip spaces
		token++;
	if (*token == '=') // and '=' character
		token++;
	while (*token == ' ') // skip spaces
		token++;

	return token;
}

static void write_properties(FILE *f, char *key, char *value)
{
	fputs(key, f);
	fputc(' ', f);
	if (strlen(value))
		fputs("= ", f);
	fputs(value, f);
	fputc('\n', f);
}

static bool read_properties(FILE *f, char *key, char *value)
{
	char buffer[256] = {0};

	if (!fgets(buffer, sizeof(buffer), f))
		return false;

	char *token = strtok(buffer, "= \t\n\r");
	if (!token)
		return false;

	strcpy(key, token);
	token = strtok_take_whole_line();
	if (token)
		strcpy(value, token);

	// stop if end of file
	return feof(f) ? false : true;
}

u32 load_properties(const char *filename, hash_node **table)
{
	FILE *f;
	char key[256] = {0};
	char value[256] = {0};

	f = fopen(filename, "rb");
	if (f == NULL)
	{
		LOG_ERROR("Cannot open file %s", filename);
		return FAIL;
	}

	LOG_INFO("Loading properties from %s", filename);

	int status = 1;

	do
	{
		status = read_properties(f, key, value);
		if (key[0] != '#')
			put_entry(table, blobify(key), blobify(value));
		else
			LOG_DEBUG("Skipping comment: %s : %s", key, value);
	} while (status);

	fclose(f);

	return SUCCESS;
}

u32 save_properties(const char *filename, hash_node **table)
{
	FILE *f = fopen(filename, "wb");
	hash_node *node;
	hash_node *next_node;

	if (f == NULL)
	{
		LOG_ERROR("Cannot open file %s", filename);
		return FAIL;
	}

	for (u32 i = 0; i < TABLE_SIZE; ++i)
	{
		node = table[i];
		while (node != NULL)
		{
			next_node = node->next;
			write_properties(f, node->key.str, node->value.str);
			node = next_node;
		}
	}

	fclose(f);

	return SUCCESS;
}
