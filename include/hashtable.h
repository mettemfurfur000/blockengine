#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "general.h"

#define TABLE_SIZE 1024

typedef struct blob
{
	union // just different names for the same thing
	{
		u8 *ptr;
		u8 *data;
		char *str; // may not be a string at all!
	};

	union
	{
		u32 length;
		u32 size;
	};
} blob;

typedef struct hash_node
{
	blob key;
	blob value;
	struct hash_node *next;
} hash_node;

unsigned long hash_function(blob key);
hash_node **alloc_table();

blob blobify(char *str);
u8 blob_dup(blob *dest, blob src);
u8 blob_cmp(blob a, blob b);

void free_table(hash_node **table);
void put_entry(hash_node **table, blob key, blob value);
blob get_entry(hash_node **table, blob key);

void print_table(hash_node **table);
void remove_entry(hash_node **table, blob key);
u64 actual_size_of_table(hash_node **table);

#endif