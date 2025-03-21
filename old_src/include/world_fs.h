#ifndef WORLD_FS
#define WORLD_FS

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "block_memory_control.h"
#include "endianless.h"

#ifdef _WIN64
#define X_P_mkdir(folder_path) mkdir(folder_path)
#else
#define X_P_mkdir(folder_path) mkdir(folder_path, 0700)
#endif

int make_full_world_path(char *filename, const world *w);
int make_full_layer_path(char *filename, const world *w, int index);
int make_full_chunk_path(char *filename, const world *w, int index, int x, int y);

int fwrite_endianless(const void *data, const int size, FILE *f);
int fread_endianless(void *data, const int size, FILE *f);

/* write/read functions */
void write_block(block *b, FILE *f);
void read_block(block *b, FILE *f);

int write_chunk(const layer_chunk *c, const char *filename);
int read_chunk(layer_chunk *c, const char *filename);

int write_layer(const world_layer *wl, const char *filename);
int read_layer(world_layer *wl, const char *filename);

int write_world(const world *w, const char *filename);
int read_world(world *w, const char *filename);
/*load/save fucntions*/
// unload means save to file and then free memory

int chunk_save(const world *w, int index, int chunk_x, int chunk_y, const layer_chunk *c);
int chunk_load(const world *w, int index, int chunk_x, int chunk_y, layer_chunk *c);
int chunk_unload(const world *w, int index, int chunk_x, int chunk_y, layer_chunk *c);

void layer_save(const world *w, const world_layer *wl);
void layer_load(const world *w, world_layer *wl, int index);
void layer_unload(world *w, world_layer *wl);

void world_save(world *w);
void world_load(world *w);
void world_unload(world *w);

// i should rename this file from world_fs.h to file_system.h

#endif