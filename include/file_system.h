#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H 1

// #include "data_io.h"
#include "level.h"

u8 save_level(level lvl);
u8 load_level(level *lvl, const char *name);
// loads a level using an existing registry to acknolwedge block ids
u8 load_level_ack_registry(level *lvl, const char *name_in, block_registry *ack_reg);

#endif // FILE_SYSTEM_H
