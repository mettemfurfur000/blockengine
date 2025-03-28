#ifndef BLOCK_REGISTRY_H
#define BLOCK_REGISTRY_H 1

// vscode itellisense wants this define so bad...
#define _DEFAULT_SOURCE 1

#ifdef _WIN64
#include "../dirent/include/dirent.h"
#else
#include <dirent.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "hashtable.h"
#include "general.h"
#include "endianless.h"
#include "block_properties.h"

#include "sdl2_basics.h"
#include "flags.h"

#include "vars.h"

#include "../vec/src/vec.h"

#define MAX_PATH_LENGTH 512

#define B_RES_FLAG_IGNORE_TYPE 0x01
#define B_RES_FLAG_RANDOM_POS 0x02
#define B_RES_FLAG_IS_FILLER 0x04
#define B_RES_AUTOMATIC_ID 0x08

typedef struct block_resources
{
	hash_node **all_fields;
	void *parent_registry;
	u64 id;
	blob vars;

	u64 ranged_id;
	vec_int_t repeat_skip;
	vec_str_t repeat_increment;

	texture block_texture;
	vec_sound_t sounds;

	char *lua_script_filename;

	// these are references to internal block data fields, not actual values for a block
	char anim_controller;	  // current animation frame / column
	char type_controller;	  // current animation type / row
	char flip_controller;	  // current type of flipping
	char rotation_controller; // current rotation angle
	u8 override_frame;		  // override type of block

	u8 frames_per_second;
	u8 flags;
} block_resources;

typedef vec_t(block_resources) block_resources_t;

typedef struct block_registry
{
	block_resources_t resources;

	const char *name;
	u64 uuid;
} block_registry;

typedef vec_t(block_registry) vec_registries_t;

#define T_UNKNOWN 0
#define T_DIGIT 1
#define T_LONG 2
#define T_INT 3
#define T_SHORT 4
#define T_BYTE 5
#define T_STRING 6

u8 length(u64 value);

void strip_digit(u8 *dest, u64 value, u32 actual_length);
u32 str_to_enum(char *type_str);
u32 get_length_to_alloc(u64 value, u32 type);

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
u8 make_block_data_from_string(const char *str_to_cpy, blob *b);

#define NOT_REQUIRED 0
#define REQUIRED 1

typedef struct
{
	void (*increment_fn)(block_resources *);
	u8 (*function)(const char *, block_resources *);
	char *name;
	u8 is_critical;
	// resource is not pushed, if:
	char *deps[4];	  // these entries are not present
	char *incompat[4]; // these entries present
	char *slots[4];			  // other entries already got said slots
} resource_entry_handler;

u32 parse_block_resources_from_file(char *file_path, block_resources *dest);
void free_block_resources(block_resources *b);

u32 is_already_in_registry(block_resources_t *reg, block_resources *br);
u32 read_block_registry(const char *name, block_registry *registry);

void sort_by_id(block_registry *b_reg);
void free_block_registry(block_registry *b_reg);

u32 read_all_registries(char *folder, vec_registries_t *dest);
block_registry *find_registry(vec_void_t src, char *name);

void debug_print_registry(block_registry *ref);

#endif