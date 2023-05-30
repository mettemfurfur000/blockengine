#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game_types.h"

#define TABLE_SIZE 32

typedef struct hash_table
{
    char *key;
    char *value;
    struct hash_table *next;
} hash_table;

hash_table *create_node(char *key, char *value)
{
    hash_table *node = (hash_table *)malloc(sizeof(hash_table));
    node->key = key;
    node->value = value;
    node->next = NULL;
    return node;
}

void free_table(hash_table **table)
{
    hash_table *current_node;
    hash_table *next_node;
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        current_node = table[i];
        while (current_node != NULL)
        {
            next_node = current_node->next;
            free(current_node);
            current_node = next_node;
        }
    }
}

unsigned long hash_function(char *key)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *key++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % TABLE_SIZE;
}

void put_entry(hash_table **table, char *key, char *value)
{
    unsigned long hash = hash_function(key);
    hash_table *node = table[hash];
    if (node == NULL)
    {
        table[hash] = create_node(key, value);
        return;
    }
    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        {
            node->value = value;
            return;
        }
        if (node->next == NULL)
        {
            node->next = create_node(key, value);
            return;
        }
        node = node->next;
    }
}

char *get_entry(hash_table **table, char *key)
{
    unsigned long hash = hash_function(key);
    hash_table *node = table[hash];
    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        {
            return node->value;
        }
        node = node->next;
    }
    return NULL;
}

void print_table(hash_table **table)
{
    hash_table *current_node;
    hash_table *next_node;
    printf("table content:\n");
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        current_node = table[i];
        while (current_node != NULL)
        {
            next_node = current_node->next;
            printf("%s:%s\n",current_node->key,current_node->value);
            current_node = next_node;
        }
    }
}

int appr_size_of(hash_table **table)
{
    int size = 0;
    hash_table *current_node;
    hash_table *next_node;

    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        current_node = table[i];
        while (current_node != NULL)
        {
            next_node = current_node->next;

            size += sizeof(*current_node);
            size += strlen(current_node->key);
            size += strlen(current_node->value);

            current_node = next_node;
        }
    }
    return size;
}

int remove_entry(hash_table **ht, char *key)
{
    unsigned long hash = hash_function(key);
    hash_table *prev = NULL;
    hash_table *current_node = ht[hash];

    while (current_node != NULL && strcmp(current_node->key, key) != 0)
    {
        prev = current_node;
        current_node = current_node->next;
    }

    if (current_node == NULL)
    {
        return FAIL;
    }

    if (prev == NULL)
    {
        ht[hash] = current_node->next;
    }
    else
    {
        prev->next = current_node->next;
    }

    free(current_node->key);
    free(current_node->value);
    free(current_node);
    return SUCCESS;
}
