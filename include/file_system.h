#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H 1

#include <stdio.h>

#include "endianless.h"
#include "general.h"
#include "tags.h"
#include "level.h"

void blob_write(blob b, FILE *f);
void blob_read(blob *b, FILE *f);

// Level saving functions
int save_level_to_file(level *l);

// Level loading functions
int load_level_from_file(level *l);

// Utility functions
int check_file_exists(const char *filename);
int create_levels_directory(void);

#endif // FILE_SYSTEM_H
