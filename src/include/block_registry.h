#ifndef BLOCK_REGISTRY
#define BLOCK_REGISTRY

// vscode itellisense wants this define so bad...
#define _DEFAULT_SOURCE 1

#ifdef _WIN64
#include "../../dirent/include/dirent.h"
#else
#include <dirent.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sdl2_basics.h"
#include "block_properties.h"
#include "endianless.h"
#include "engine_types.h"
#include "block_memory_control.h"
#include "flags.h"

#include "../../vec/src/vec.h"

#define MAX_PATH_LENGTH 512

#define B_RES_FLAG_IGNORE_TYPE 0x01
#define B_RES_FLAG_RANDOM_POS 0x02
#define B_RES_FLAG_IS_FILLER 0x04

typedef struct block_resources
{
	hash_table **all_fields;
	block block_sample;

	texture block_texture;

	char *lua_script_filename;

	char anim_controller; // controls which character inside of block controls current animation mode of block
	char type_controller; // controls same thing but for animation type/state
	byte frames_per_second;

	byte flags;
} block_resources;

typedef vec_t(block_resources) block_registry_t;

#define T_UNKNOWN 0
#define T_DIGIT 1
#define T_LONG 2
#define T_INT 3
#define T_SHORT 4
#define T_BYTE 5
#define T_STRING 6

byte length(long long value);

void strip_digit(byte *dest, long long value, int actual_length);
int str_to_enum(char *type_str);
int get_length_to_alloc(long long value, int type);

// format:

// { <character> <type> = <value>
//   <character> <type> = <value>
//   ...
//   <character> <type> = <value> }

// acceptable types: digit, long, int, short, byte, string
//
// strings ends with \n character, rember
//
// if type donesnt match, value will be copied as strings
//
// digits will be stripped for saving space, keep it in mind
//
// other numerical types will save their length in bytes, even if you pass it with value of 0
int make_block_data_from_string(const char *str_to_cpy, byte **out_data_ptr);

#define NOT_REQUIRED 0
#define REQUIRED 1

typedef struct
{
	int (*function)(const char *, block_resources *);
	char *name;
	byte is_critical;
	char *dependencies[4];
	char *incompabilities[4];
} resource_entry_handler;

int parse_block_resources_from_file(const char *file_path, block_resources *dest);
void free_block_resources(block_resources *b);

int is_already_in_registry(block_registry_t *reg, block_resources *br);
int read_block_registry(const char *folder, block_registry_t *reg);

int __b_cmp(const void *a, const void *b);

void sort_by_id(block_registry_t *b_reg);
void free_block_registry(block_registry_t *b_reg);

#endif