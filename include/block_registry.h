#ifndef BLOCK_REGISTRY_H
#define BLOCK_REGISTRY_H 1

// #define _DEFAULT_SOURCE 1

#include "general.h"
#include "hashtable.h"
#include "sdl2_basics.h"

#include "vec/src/vec.h"

#define RESOURCE_FLAG_IGNORE_TYPE 0b00000001
#define RESOURCE_FLAG_RANDOM_POS 0b00000010
#define RESOURCE_FLAG_IS_FILLER 0b00000100
#define RESOURCE_FLAG_AUTO_ID 0b00001000
#define RESOURCE_FLAG_RANGED 0b00010000

#define REGISTRY_VERSION_MAGIC "BRG2"

typedef struct component_blob_entry
{
	char *name;
	unsigned char *blob;
	u32 blob_size;
} component_blob_entry;

typedef vec_t(component_blob_entry) component_blob_t;

typedef struct block_resources
{
	hash_node **all_fields;
	void *parent_registry;
	u64 id;

	blob vars_sample;	   // sample vars blob for this block, ready to use
	i32 vars_offsets[256]; // pre-computed offsets for each letter (-1 = not present)

	image *img;
	atlas_info info;

	char *texture_filename;
	char *lua_script_filename;
	// compiled lua bytecode embedded in .brg (owned by this struct)
	unsigned char *lua_script_blob;
	u32 lua_script_blob_size;

	vec_str_t component_names;

	vec_sound_t sounds;

	vec_int_t input_refs;
	vec_str_t input_names;

i32 input_tick_ref; // which input causes a tick update, as a lua ref
	u32 entity_tick_ref; // same but when the block is a part of an entity
	u32 entity_collision_ref; // called when entity collides with a block
	// 1 - 3x3 tileset, 4 neighbours, lines default to center tile
	// 2 - 4x4, 4 neighbours, full coverage on all cases
	// 3 - 47 autotile scheme, 8 neighbour scheme
	u8 autotile_type;
	// char autotile_update_key;
	// char autotile_cache_key;

	char anim_controller;
	char type_controller;
	char flip_controller;
	char rotation_controller;
	char offset_x_controller;
	char offset_y_controller;
	char interp_timestamp_controller;

	u32 interp_takes;

	u8 override_frame;

	u8 frames_per_second;
	u8 flags;
} block_resources;

typedef vec_t(block_resources) block_resources_t;

typedef struct block_registry
{
	block_resources_t resources;
	component_blob_t component_blobs;

	const char *name;
	image *atlas;

	GLuint atlas_texture_uid;
	u64 uuid;
} block_registry;

typedef vec_t(block_registry) vec_registries_t;

#define NOT_REQUIRED 0
#define REQUIRED 1

typedef struct
{
	u8 (*function)(const char *, block_resources *);
	char *name;

	u8 is_critical; // resource is not pushed if its absend, or if:

	char *deps[4];	   // these entries are not present
	char *incompat[4]; // these entries present
	char *slots[4];	   // other entries already got said slots
} resource_entry_handler;

void free_block_resources(block_resources *b);

u32 is_already_in_registry(block_resources_t *reg, block_resources *br);
u32 read_block_registry(block_registry *reg_ref, const char *folder_name);

// returns the block name derived from source_filename (basename minus .blk),
// malloc'd, caller frees. NULL if the resource has no source_filename.
char *block_source_name(block_resources *b);

// returns the id of the block with the given name, or FAIL if not found
u64 block_registry_find_id_by_name(block_registry *reg, const char *name);

void sort_by_id(block_resources_t *reg);
void free_block_registry(block_registry *b_reg);

// recompute vars_offsets from the current vars_sample (call after components
// mutate vars at load time)
void rebuild_vars_offsets(block_resources *res);

// returns a pointer to the compiled component blob entry for the given name,
// or NULL if the component was not compiled into this registry.
component_blob_entry *registry_find_component_blob(block_registry *reg, const char *name);

u32 read_all_registries(char *folder, vec_registries_t *dest);
block_registry *find_registry(vec_void_t src, char *name);

void debug_print_registry(block_registry *ref);

// extensions for reading/writing registries

u8 registry_save(block_registry *b);
block_registry *registry_load(const char *name);

#endif