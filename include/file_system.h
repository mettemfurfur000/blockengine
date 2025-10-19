#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H 1

#include "general.h"
#include "level.h"

#include <zlib.h>

#define COMPRESS_LEVELS 1

u8 save_level(level lvl);
u8 load_level(level *lvl, const char *name);

#endif // FILE_SYSTEM_H
