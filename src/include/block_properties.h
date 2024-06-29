#ifndef BLOCK_PROPS
#define BLOCK_PROPS

#include "hash_table.h"

char *strtok_take_whole_line();

void write_properties(FILE *f, char *key, char *value);
int read_properties(FILE *f, char *key, char *value);

int load_properties(const char *filename, hash_table **table);
int save_properties(const char *filename, hash_table **table);

#endif