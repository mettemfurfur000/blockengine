#ifndef BLOCK_PROPS
#define BLOCK_PROPS 1

#include "hash_table.c"

char *strtok_take_all_line()
{
	char *token = strtok(NULL, "\n"); // take everything until first newline character
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

void write_properties(FILE *f, char *key, char *value)
{
	fputs(key, f);
	fputs(" ", f);
	if (strlen(value))
		fputs("= ", f);
	fputs(value, f);
	fputc('\n', f);
}

int read_properties(FILE *f, char *key, char *value)
{
	// thanks https://t.me/codemaniacbot
	char buffer[512] = {0};
	if (fgets(buffer, sizeof(buffer), f))
	{
		char *token = strtok(buffer, "= \t\n");
		if (token)
		{
			strcpy(key, token);
			token = strtok_take_all_line();
			if (token)
			{
				strcpy(value, token);
			}
		}
	}
	return !feof(f);
}

int load_properties(char *filename, hash_table **table)
{
	FILE *f;
	char key[256] = {0};
	char value[256] = {0};

	f = fopen(filename, "rb");
	if (f == NULL)
	{
		printf("Cannot open file %s\n", filename);
		return FAIL;
	}

	while (read_properties(f, key, value))
	{
		put_entry(table, key, value);
	}

	fclose(f);

	return SUCCESS;
}

int save_properties(char *filename, hash_table **table)
{
	FILE *f = fopen(filename, "wb");
	hash_table *node;
	hash_table *next_node;

	if (f == NULL)
	{
		printf("Cannot open file %s\n", filename);
		return FAIL;
	}

	for (int i = 0; i < TABLE_SIZE; ++i)
	{
		node = table[i];
		while (node != NULL)
		{
			next_node = node->next;
			write_properties(f, node->key, node->value);
			node = next_node;
		}
	}

	fclose(f);

	return SUCCESS;
}

#endif