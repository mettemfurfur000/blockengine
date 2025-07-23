#ifndef HASHTABLE_H
#define HASHTABLE_H 1

#include <unistd.h>

#define HASHTABLE_DEBUG_LOGGING 0

#include "general.h"

#define TABLE_SIZE 31

typedef struct blob
{
    union // just different names for the same thing
    {
        u8 *ptr;
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
void blob_generate(blob *b, u32 seed);

blob blobify(const char *str);
u8 blob_dup(blob *dest, blob src);
u8 blob_cmp(blob a, blob b);

void free_table(hash_node **table);
void put_entry(hash_node **table, blob key, blob value);
blob get_entry(hash_node **table, blob key);

void print_node(hash_node *node, const char *context);
void print_table(hash_node **table);
void remove_entry(hash_node **table, blob key);

u64 actual_size_of_table(hash_node **table);
u64 table_elements(hash_node **table);

#define HASHTABLE_FOREACH_EXEC(table, node_name, code)                                                                 \
    {                                                                                                                  \
        CHECK_PTR(table);                                                                                              \
        hash_node *node_name;                                                                                          \
        for (u32 __i = 0; __i < TABLE_SIZE; ++__i)                                                                     \
        {                                                                                                              \
            node_name = table[__i];                                                                                    \
            while (node_name != NULL)                                                                                  \
            {                                                                                                          \
                code;                                                                                                  \
                node_name = node_name->next;                                                                           \
            }                                                                                                          \
        }                                                                                                              \
    }

#endif