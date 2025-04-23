#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H 1

#include <stdio.h>

#include "general.h"
#include "level.h"

void blob_write(blob b, FILE *f);
blob blob_read(FILE *f);

u8 save_level(level lvl);
u8 load_level(level *lvl, char *name);

#endif // FILE_SYSTEM_H
