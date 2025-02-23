#include "../include/hashtable.h"

unsigned long hash_function(blob key)
{
	unsigned long hash = 5381;

	for (int i = 0; i < key.size; i++)
		hash = ((hash << 5) + hash) + key.ptr[i];

	return hash % TABLE_SIZE;
}

void blob_generate(blob *b, u32 seed)
{
	SAFE_FREE(b->ptr);

	b->size = seed % 32;
	b->ptr = malloc(b->size);

	srand(seed);

	for (int i = 0; i < b->size; i++)
		b->ptr[i] = (char)(rand() % 256);
}

blob blobify(char *str)
{
	blob b = {.str = str, .length = strlen(str) + 1};
	return b;
}

u8 blob_dup(blob *dest, blob src)
{
	CHECK_PTR(dest);

	SAFE_FREE(dest->ptr)

	dest->ptr = malloc(src.size);
	memcpy(dest->ptr, src.ptr, src.size);

	dest->size = src.size;

	LOG_DEBUG("blob %.*s (%d) copied to %p", src.size, src.ptr, src.size, dest->ptr);

	return SUCCESS;
}

u8 blob_cmp(blob a, blob b)
{
	if (a.size != b.size)
		return -1;

	if (a.ptr == b.ptr)
		return 0;

	for (int i = 0; i < a.size; i++)
		if (a.ptr[i] != b.ptr[i])
			return a.ptr[i] - b.ptr[i];

	return 0;
}

hash_node *alloc_node()
{
	return (hash_node *)calloc(1, sizeof(hash_node));
}

void free_node(hash_node *node)
{
#if HASHTABLE_DEBUG_LOGGING == 1
	print_node(node, "freeing");
#endif

	SAFE_FREE(node->key.ptr)
	SAFE_FREE(node->value.ptr)
	SAFE_FREE(node);
}

void copy_key(hash_node *node, blob key)
{
	blob_dup(&node->key, key);
#if HASHTABLE_DEBUG_LOGGING == 1
	print_node(node, "key copied");
#endif
}

void copy_value(hash_node *node, blob value)
{
	blob_dup(&node->value, value);

#if HASHTABLE_DEBUG_LOGGING == 1
	print_node(node, "value copied");
#endif
}

void copy_all(hash_node *node, blob key, blob value)
{
	blob_dup(&node->key, key);
	blob_dup(&node->value, value);

#if HASHTABLE_DEBUG_LOGGING == 1
	print_node(node, "all copied");
#endif
}

hash_node **alloc_table()
{
	hash_node **t = (hash_node **)malloc(TABLE_SIZE * sizeof(hash_node *));
	memset(t, 0, TABLE_SIZE * sizeof(hash_node *));

	LOG_DEBUG("Allocated table of size %d, ptr %p", TABLE_SIZE, t);

	return t;
}

void free_table(hash_node **table)
{
	CHECK_PTR_NORET(table);

	LOG_DEBUG("Freeing table of size %d, ptr %p", TABLE_SIZE, table);

	hash_node *node;
	hash_node *next_node;

	for (int i = 0; i < TABLE_SIZE; ++i)
	{
		node = table[i];
		while (node != NULL)
		{
			next_node = node->next;
			free_node(node);
			node = next_node;
		}
	}
}

void put_entry(hash_node **table, blob key, blob value)
{
	CHECK_PTR_NORET(table);

	unsigned long hash = hash_function(key);
	hash_node *node = table[hash];

	if (node == NULL)
	{
		node = alloc_node();
		copy_all(node, key, value);
		table[hash] = node;
		return;
	}

	while (node != NULL)
	{
		if (blob_cmp(node->key, key) == 0)
		{
			copy_value(node, value);
			return;
		}

		if (node->next == NULL)
		{
			node->next = alloc_node();
			copy_all(node->next, key, value);
			return;
		}

		node = node->next;
	}
}

blob get_entry(hash_node **table, blob key)
{
	if (!table)
		return key;

	blob ret = {};

	unsigned long hash = hash_function(key);
	hash_node *node = table[hash];

	while (node != NULL)
	{
		if (blob_cmp(node->key, key) == 0)
			return node->value;

		node = node->next;
	}

	return ret;
}

void print_node(hash_node *node, const char *context)
{
	LOG_DEBUG("%s %.*s [%d] : %.*s [%d], %p -> %p",
			  context,
			  node->key.size,
			  node->key.ptr,
			  node->key.size,
			  node->value.size, node->value.ptr,
			  node->value.size,
			  node->key.ptr,
			  node->value.ptr);
}

void print_table(hash_node **table)
{
	hash_node *node;
	LOG_DEBUG("table content:");

	for (int i = 0; i < TABLE_SIZE; ++i)
	{
		node = table[i];
		while (node != NULL)
		{
#if HASHTABLE_DEBUG_LOGGING == 1
			print_node(node, "printing");
#endif
			node = node->next;
		}
	}
}

void remove_entry(hash_node **table, blob key)
{
	CHECK_PTR_NORET(table);

	unsigned long hash = hash_function(key);
	hash_node *prev = NULL;
	hash_node *node = table[hash];

	while (node != NULL && blob_cmp(node->key, key) != 0)
	{
		prev = node;
		node = node->next;
	}

	if (node == NULL)
		return;

	if (prev == NULL)
		table[hash] = node->next;
	else
		prev->next = node->next;

	free_node(node);
}

u64 actual_size_of_table(hash_node **table)
{
	CHECK_PTR(table);

	u64 size = 0;
	hash_node *node;

	for (int i = 0; i < TABLE_SIZE; ++i) // goes wide
	{
		node = table[i];

		while (node != NULL)
		{
			size += node->key.size + node->value.size;

			node = node->next;
		}
	}

	return size;
}

u64 table_elements(hash_node **table)
{
	CHECK_PTR(table);

	u64 count = 0;

	hash_node *node;
	for (int i = 0; i < TABLE_SIZE; ++i)
	{
		node = table[i];
		while (node != NULL)
		{
			count++;
			node = node->next;
		}
	}

	return count;
}