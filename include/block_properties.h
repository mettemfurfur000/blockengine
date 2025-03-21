#ifndef BLOCK_PROPERTIES_H
#define BLOCK_PROPERTIES_H 1

#include "hashtable.h"

char *strtok_take_whole_line();

void write_properties(FILE *f, char *key, char *value);
int read_properties(FILE *f, char *key, char *value);

int load_properties(const char *filename, hash_node **table);
int save_properties(const char *filename, hash_node **table);

#endif