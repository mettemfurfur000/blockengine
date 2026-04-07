#ifndef BLOCK_PROPERTIES_H
#define BLOCK_PROPERTIES_H 1

#include "hashtable.h"

char *strtok_take_whole_line();

u32 load_properties(const char *filename, hash_node **table);
u32 save_properties(const char *filename, hash_node **table);

#endif