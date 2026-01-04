#ifndef BLOCK_REGISTRY_H
#define BLOCK_REGISTRY_H 1

// #define _DEFAULT_SOURCE 1

#include "general.h"
#include "hashtable.h"
#include "sdl2_basics.h"

#include "../vec/src/vec.h"

#define RESOURCE_FLAG_IGNORE_TYPE 0b00000001
#define RESOURCE_FLAG_RANDOM_POS 0b00000010
#define RESOURCE_FLAG_IS_FILLER 0b00000100
#define RESOURCE_FLAG_AUTO_ID 0b00001000
#define RESOURCE_FLAG_RANGED 0b00010000

typedef struct block_resources
{
    hash_node **all_fields;
    void *parent_registry;
    u64 id;

    blob vars_sample;

    u64 repeat_times;
    vec_int_t repeat_skip;
    vec_str_t repeat_increment;

    image *img;
    atlas_info info;

    char *texture_filename;
    char *lua_script_filename;

    vec_sound_t sounds;

    vec_int_t input_refs;
    vec_str_t input_names;

    i32 input_tick_ref;
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
    void (*increment_fn)(block_resources *);
    u8 (*function)(const char *, block_resources *);
    char *name;

    u8 is_critical; // resource is not pushed if its absend, or if:

    char *deps[4];     // these entries are not present
    char *incompat[4]; // these entries present
    char *slots[4];    // other entries already got said slots
} resource_entry_handler;

void free_block_resources(block_resources *b);

u32 is_already_in_registry(block_resources_t *reg, block_resources *br);
u32 read_block_registry(block_registry *reg_ref, const char *folder_name);

void sort_by_id(block_resources_t *reg);
void free_block_registry(block_registry *b_reg);

u32 read_all_registries(char *folder, vec_registries_t *dest);
block_registry *find_registry(vec_void_t src, char *name);

void debug_print_registry(block_registry *ref);

#endif