#ifndef HASH_TABLE
#define HASH_TABLE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "engine_types.h"

#define TABLE_SIZE 113

typedef struct hash_table
{
	char *key;
	char *value;
	struct hash_table *next;
} hash_table;

unsigned long hash_function(char *key);

hash_table *alloc_node();

void free_node(hash_table *node);

void copy_key(hash_table *node, char *key);
void copy_value(hash_table *node, char *value);
void copy_all(hash_table *node, char *key, char *value);

hash_table **alloc_table();

void free_table(hash_table **table);
void put_entry(hash_table **table, char *key, char *value);
char *get_entry(hash_table **table, char *key);

void print_table(hash_table **table);
int remove_entry(hash_table **table, char *key);
int actual_size_of_table(hash_table **table);

void fill_test_key(char *key, int num);
void fill_test_val(char *val, int num);
void put_random_entry(hash_table **table, int seed);
int verify_random_entry(hash_table **table, int seed);

#endif