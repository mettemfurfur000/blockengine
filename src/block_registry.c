#include "../include/block_registry.h"
#include "../include/atlas_builder.h"
#include "../include/block_properties.h"
#include "../include/flags.h"
#include "../include/scripting.h"
#include "../include/uuid.h"
#include "../include/vars.h"
#include "../include/vars_utils.h"
#include "../include/folder_structure.h"

#ifdef _WIN64
#include "../dirent/include/dirent.h"
#else
#include <dirent.h>
#endif

// #include <ctype.h>
#include <stdlib.h>
#include <string.h>

u8 read_int_list(const char *str, vec_int_t *dest)
{
    CHECK_PTR(str)
    CHECK_PTR(dest)

    char *dup = strdup(str);

    char *token = strtok(dup, " \n"); // consume start token
    if (strcmp(token, "[") != 0)
    {
        free(dup);
        return FAIL;
    }

    u64 value;

    while (1)
    {
        token = strtok(NULL, " \n");
        if (!token)
        {
            free(dup);
            return FAIL;
        }

        if (strcmp(token, "]") == 0)
        {
            free(dup);
            return SUCCESS;
        }

        value = atoll(token);

        (void)vec_push(dest, value);
    }

    free(dup);
    return SUCCESS;
}

u8 read_str_list(const char *str, vec_str_t *dest)
{
    CHECK_PTR(str)
    CHECK_PTR(dest)

    char *dup = strdup(str);

    char *token = strtok(dup, " \n"); // consume start token
    if (strcmp(token, "[") != 0)
    {
        free(dup);
        return FAIL;
    }

    char *value = NULL;

    while (1)
    {
        token = strtok(NULL, " \n");
        if (!token)
        {
            free(dup);
            return FAIL;
        }

        if (strcmp(token, "]") == 0)
        {
            free(dup);
            return SUCCESS;
        }

        value = strdup(token);

        (void)vec_push(dest, value);
    }

    free(dup);

    return SUCCESS;
}

/*
all block resources handlers must return SUCCESS if they handled data

if data contains a special clean token, handler will try to free/clear data from
dest resource

also handlers must return FAIL if they cant handle data or if data is invalid
*/

#define DECLARE_DEFAULT_FLAG_HANDLER(name, flag)                                                                       \
    u8 block_res_##name##_handler(const char *data, block_resources *dest)                                             \
    {                                                                                                                  \
        FLAG_CONFIGURE(dest->flags, flag, atoi(data) ? 1 : 0, strcmp(data, clean_token) == 0)                          \
        return SUCCESS;                                                                                                \
    }

#define DECLARE_DEFAULT_CHAR_FIELD_HANDLER(name)                                                                       \
    u8 block_res_##name##_handler(const char *data, block_resources *dest)                                             \
    {                                                                                                                  \
        if (strcmp(data, clean_token) == 0)                                                                            \
        {                                                                                                              \
            dest->name = 0;                                                                                            \
            return SUCCESS;                                                                                            \
        }                                                                                                              \
        dest->name = data[0];                                                                                          \
        return SUCCESS;                                                                                                \
    }

#define DECLARE_DEFAULT_BYTE_FIELD_HANDLER(name)                                                                       \
    u8 block_res_##name##_handler(const char *data, block_resources *dest)                                             \
    {                                                                                                                  \
        if (strcmp(data, clean_token) == 0)                                                                            \
        {                                                                                                              \
            dest->name = 0;                                                                                            \
            return SUCCESS;                                                                                            \
        }                                                                                                              \
        dest->name = atoi(data);                                                                                       \
        return SUCCESS;                                                                                                \
    }

#define DECLARE_DEFAULT_LONG_FIELD_HANDLER(name)                                                                       \
    u8 block_res_##name##_handler(const char *data, block_resources *dest)                                             \
    {                                                                                                                  \
        if (strcmp(data, clean_token) == 0)                                                                            \
        {                                                                                                              \
            dest->name = 0;                                                                                            \
            return SUCCESS;                                                                                            \
        }                                                                                                              \
        dest->name = atol(data);                                                                                       \
        return SUCCESS;                                                                                                \
    }

#define DECLARE_DEFAULT_INT_FIELD_HANDLER(name)                                                                        \
    u8 block_res_##name##_handler(const char *data, block_resources *dest)                                             \
    {                                                                                                                  \
        if (strcmp(data, clean_token) == 0)                                                                            \
        {                                                                                                              \
            dest->name = 0;                                                                                            \
            return SUCCESS;                                                                                            \
        }                                                                                                              \
        dest->name = atoi(data);                                                                                       \
        return SUCCESS;                                                                                                \
    }

#define DECLARE_DEFAULT_INCREMENTOR(name)                                                                              \
    void block_res_##name##_incrementor(block_resources *dest)                                                         \
    {                                                                                                                  \
        dest->name++;                                                                                                  \
    }

#define DECLARE_DEFAULT_STR_VEC_HANDLER(name)                                                                          \
    u8 block_res_##name##_vec_str_handler(const char *data, block_resources *dest)                                     \
    {                                                                                                                  \
        if (strcmp(data, clean_token) == 0)                                                                            \
        {                                                                                                              \
            int i;                                                                                                     \
            char *val;                                                                                                 \
            vec_foreach(&dest->name, val, i)                                                                           \
            {                                                                                                          \
                SAFE_FREE(val);                                                                                        \
            }                                                                                                          \
            if (dest->name.data)                                                                                       \
                vec_deinit(&dest->name);                                                                               \
            else                                                                                                       \
                vec_init(&dest->name);                                                                                 \
        }                                                                                                              \
        read_str_list(data, &dest->name);                                                                              \
        return SUCCESS;                                                                                                \
    }

const static char clean_token[] = "??clean??";

DECLARE_DEFAULT_LONG_FIELD_HANDLER(id)
DECLARE_DEFAULT_LONG_FIELD_HANDLER(repeat_times)
// DECLARE_DEFAULT_LONG_FIELD_HANDLER(repeat_skip)

u8 block_res_repeat_skip_handler(const char *data, block_resources *dest)
{
    if (strcmp(data, clean_token) == 0)
    {
        if (dest->repeat_skip.data)
            vec_deinit(&dest->repeat_skip);
        else
            vec_init(&dest->repeat_skip);

        return SUCCESS;
    }

    read_int_list(data, &dest->repeat_skip);

    return SUCCESS;
}

DECLARE_DEFAULT_STR_VEC_HANDLER(repeat_increment)
u8 block_res_sounds_vec_handler(const char *data, block_resources *dest)
{
    if (strcmp(data, clean_token) == 0)
    {
        int i;
        sound val;
        vec_foreach(&dest->sounds, val, i)
        {
            SAFE_FREE(val.filename);
            free_sound(&val);
        }

        if (dest->sounds.data)
            vec_deinit(&dest->sounds);
        else
            vec_init(&dest->sounds);
    }

    vec_str_t tmp = {};

    read_str_list(data, &tmp);

    CHECK_PTR(dest->parent_registry)

    block_registry *b_reg = (block_registry *)dest->parent_registry;

    CHECK_PTR(b_reg->name)

    char sound_full_path[MAX_PATH_LENGTH] = {};

    for (u32 i = 0; i < tmp.length; i++)
    {
        sound t = {};

        snprintf(sound_full_path, sizeof(sound_full_path),
                 FOLDER_REG SEPARATOR_STR "%s" SEPARATOR_STR FOLDER_REG_SND SEPARATOR_STR "%s", b_reg->name,
                 tmp.data[i]);

        sound_load(&t, sound_full_path);
        (void)vec_push(&dest->sounds, t);
    }

    return SUCCESS;
}

DECLARE_DEFAULT_BYTE_FIELD_HANDLER(override_frame)
DECLARE_DEFAULT_INCREMENTOR(override_frame)

u8 block_res_data_handler(const char *data, block_resources *dest)
{
    return strcmp(data, clean_token) == 0 ? vars_free(&dest->vars_sample) : vars_parse(data, &dest->vars_sample);
}

u8 block_res_texture_handler(const char *data, block_resources *dest)
{
    if (strcmp(data, clean_token) == 0)
    {
        free_image(dest->img);
        dest->img = 0;

        SAFE_FREE(dest->texture_filename);

        return SUCCESS;
    }

    CHECK_PTR(dest->parent_registry)

    block_registry *b_reg = (block_registry *)dest->parent_registry;

    CHECK_PTR(b_reg->name)

    char texture_full_path[MAX_PATH_LENGTH] = {};
    snprintf(texture_full_path, sizeof(texture_full_path),
             FOLDER_REG SEPARATOR_STR "%s" SEPARATOR_STR FOLDER_REG_TEX SEPARATOR_STR "%s", b_reg->name, data);

    // old way makes it a texture right away and frees ma precius pixels! we
    // need deez image pixels to make some useful work in the atlas_builder
    // later!

    // return texture_load(&dest->block_texture, texture_full_path);
    // return load_image_alt(&dest->img, &dest->block_texture,
    // texture_full_path);

    dest->img = load_image(texture_full_path);

    if (dest->img == NULL)
    {
        LOG_ERROR("block_res_texture_handler : failed to load an image for the "
                  "texture: %s",
                  texture_full_path);
        return FAIL;
    }

    record_atlas_info(&dest->info, dest->img);

    dest->texture_filename = strdup(texture_full_path);

    return SUCCESS;
}

u8 block_res_fps_handler(const char *data, block_resources *dest)
{
    if (strcmp(data, clean_token) == 0)
    {
        dest->frames_per_second = 0;
        return SUCCESS;
    }

    u32 fps = atoi(data);
    dest->frames_per_second = fps;
    return SUCCESS;
}

DECLARE_DEFAULT_FLAG_HANDLER(ignore_type, RESOURCE_FLAG_IGNORE_TYPE)
DECLARE_DEFAULT_FLAG_HANDLER(position_based_type, RESOURCE_FLAG_RANDOM_POS)
DECLARE_DEFAULT_FLAG_HANDLER(automatic_id, RESOURCE_FLAG_AUTO_ID)

DECLARE_DEFAULT_CHAR_FIELD_HANDLER(anim_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(type_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(flip_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(rotation_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(offset_x_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(offset_y_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(interp_timestamp_controller)
// DECLARE_DEFAULT_CHAR_FIELD_HANDLER(interp_takes_controller)

DECLARE_DEFAULT_INT_FIELD_HANDLER(interp_takes)

u8 block_res_lua_script_handler(const char *data, block_resources *dest)
{
    if (strcmp(data, clean_token) == 0)
    {
        free(dest->lua_script_filename);
        return SUCCESS;
    }

    u32 len = strlen(data);
    dest->lua_script_filename = malloc(len + 1);
    memcpy(dest->lua_script_filename, data, len + 1);

    return SUCCESS;
}

DECLARE_DEFAULT_STR_VEC_HANDLER(input_names)

// u8 block_res_parent_handler(const char *data, block_resources *dest)
// {
// 	return SUCCESS;
// }

/* end of block resource handlers */

const static resource_entry_handler res_handlers[] = {
    // every block needs an identity, or its just a thin air
    // {NULL, &block_res_parent_handler, "parent", NOT_REQUIRED, {}, {}, {}},
    {                                 NULL,&block_res_id_handler,"id", NOT_REQUIRED,{},{},  {"identity"}                                                                                                                                                   },
    {                                 NULL,             &block_res_automatic_id_handler,        "automatic_id", NOT_REQUIRED,               {}, {},      {"identity"}},
    // every block needs a visual representation
    {                                 NULL,                  &block_res_texture_handler,             "texture",     REQUIRED,               {}, {},                {}},
    // every block needs a sound, right?
    // may set to REQUIRED later
    {                                 NULL,               &block_res_sounds_vec_handler,              "sounds", NOT_REQUIRED,               {}, {},                {}},
    // dangerous: will replicate the same block multiple times until it reaches
    // the said id
    // useful if you just want to have a bunch of blocks with the same texture
    // and locked type
    {                                 NULL,             &block_res_repeat_times_handler,        "repeat_times", NOT_REQUIRED,               {}, {},                {}},
    // if id accurs on this list while ranging, all increments will pass but no
    // blocks will be pasted
    {                                 NULL,              &block_res_repeat_skip_handler,         "repeat_skip", NOT_REQUIRED, {"repeat_times"}, {},                {}},
    // this controls what variables shoud be incremented as id_range progresses
    // forward
    // triggers them and passes id of current ranged block as a string
    {                                 NULL, &block_res_repeat_increment_vec_str_handler,    "repeat_increment", NOT_REQUIRED, {"repeat_times"}, {},                {}},
    // unique data goes here. Each block will have a copy of these values on
    // paste
    {                                 NULL,                     &block_res_data_handler,                "vars", NOT_REQUIRED,               {}, {},                {}},

    // graphics-related stuff
    // frames
    {                                 NULL,                      &block_res_fps_handler,                 "fps", NOT_REQUIRED,               {}, {}, {"frame_control"}},
    // changes frame of a block N times per second. Not a
    // float!
    {                                 NULL,          &block_res_anim_controller_handler,    "frame_controller", NOT_REQUIRED,         {"vars"}, {}, {"frame_control"}},
    // directly controls the frame of a texture, so it cant
    // used with fps
    {                                 NULL,      &block_res_position_based_type_handler,        "random_frame", NOT_REQUIRED,               {}, {}, {"frame_control"}},
    // randomizes frame of a block based on its location

    {&block_res_override_frame_incrementor,
     &block_res_override_frame_handler,
     "override_frame", NOT_REQUIRED,
     {},
     {},
     {"frame_control"}                                                                                                                                               },

    // directly sets its type from block resources - solid
    // as rock
    // flips, untested
    {                                 NULL,          &block_res_flip_controller_handler,     "flip_controller", NOT_REQUIRED,         {"vars"}, {},                {}},
    // untested
    {                                 NULL,      &block_res_rotation_controller_handler, "rotation_controller", NOT_REQUIRED,         {"vars"}, {},                {}},
    {                                 NULL,      &block_res_offset_x_controller_handler, "offset_x_controller", NOT_REQUIRED,         {"vars"}, {},                {}},
    {                                 NULL,      &block_res_offset_y_controller_handler, "offset_y_controller", NOT_REQUIRED,         {"vars"}, {},                {}},
    {                                 NULL,
     &block_res_interp_timestamp_controller_handler,
     "interp_timestamp_controller", NOT_REQUIRED,
     {"vars"},
     {},
     {}                                                                                                                                                              },
    {                                 NULL,
     &block_res_interp_takes_handler,
     "interp_takes", NOT_REQUIRED,
     {"vars", "interp_timestamp_controller"},
     {},
     {}                                                                                                                                                              },
    // block type things
    {                                 NULL,          &block_res_type_controller_handler,     "type_controller", NOT_REQUIRED,         {"vars"}, {},  {"type_control"}},
    // reads its type from a block data - can be controlled
    // with scripts
    {                                 NULL,              &block_res_ignore_type_handler,         "ignore_type", NOT_REQUIRED,               {}, {},  {"type_control"}},
    // ignores type and treats each 16x16 square on a
    // texture as a frame
    // block logic zone
    // custom script file, executed on level creation
    {                                 NULL,               &block_res_lua_script_handler,              "script", NOT_REQUIRED,               {}, {},                {}},
    // block inputs
    {                                 NULL,      &block_res_input_names_vec_str_handler,              "inputs", NOT_REQUIRED,       {"script"}, {},                {}}
};

const u32 TOTAL_HANDLERS = sizeof(res_handlers) / sizeof(*res_handlers);

// this is probably not really required sinc im comparing pointers to string
// literals, but that might change in the future
const char *vec_str_find(vec_str_t *vec, const char *str)
{
    u32 ch;
    char *filled;
    vec_foreach(vec, filled, ch) if (strcmp(filled, str) == 0) return str;
    return NULL;
}

// returns an unfilfilled dependency, or null if everything is ok
const char *check_deps(resource_entry_handler *handler, vec_str_t *seen_entries)
{
    for (u32 j = 0; j < sizeof(handler->deps) / sizeof(void *); j++)
    {
        char *dep = handler->deps[j];
        if (!dep)
            continue;

        if (seen_entries->length == 0)
            return dep;

        const char *result = vec_str_find(seen_entries, dep);
        if (result == NULL)
            return result;
    }

    return NULL;
}

// same as above but errors if an entry is present
const char *check_incompat(resource_entry_handler *handler, vec_str_t *seen_entries)
{
    for (u32 j = 0; j < sizeof(handler->incompat) / sizeof(void *); j++)
    {
        char *inc = handler->incompat[j];
        if (!inc)
            continue;

        if (seen_entries->length == 0) // this entry is first and cant be incompatible with cosmic void
            return NULL;

        const char *result = vec_str_find(seen_entries, inc);
        if (result)
            return result;
    }

    return NULL;
}

const char *check_slot(resource_entry_handler *handler, vec_str_t *seen_entries, vec_str_t *filled_slots)
{
    for (u32 j = 0; j < sizeof(handler->slots) / sizeof(void *); j++)
    {
        char *slot = handler->slots[j];
        if (!slot)
            continue;

        const char *result = vec_str_find(filled_slots, slot);
        if (result)
            return result;

        (void)vec_push(filled_slots, slot);
    }

    return NULL;
}

u32 parse_block_resources_from_file(const char *file_path, block_resources *dest)
{
    hash_node **properties = alloc_table();
    u32 status = SUCCESS;

    if (load_properties(file_path, properties) != SUCCESS)
    {
        LOG_ERROR("Error loading properties: %s", file_path);

        free_table(properties);
        return FAIL;
    }

    put_entry(properties, blobify("source_filename"), blobify(file_path));

    // also include a full file in block resources, for whatever reason
    dest->all_fields = properties;

    blob entry = {};

    char copy_buffer[1024] = {};

    vec_str_t seen_entries;
    vec_init(&seen_entries);

    vec_str_t filled_slots;
    vec_init(&filled_slots);

    for (u32 i = 0; i < TOTAL_HANDLERS; i++)
    {
        resource_entry_handler h = res_handlers[i];

        entry = get_entry(properties, blobify(h.name));

        if (!entry.ptr)
        {
            if (h.is_critical)
            {
                LOG_ERROR("\"%s\" : critical entry \"%s\" is not present", file_path, h.name);
                status = FAIL;
                break;
            }
            continue;
        }
        // fix string to be null terminated
        // this line of code caused alot of headaches for me, i hate myself
        entry.str[entry.length - 1] = '\0';
        // copy string to a buffer so strtok can work without modifying the
        // original string
        memcpy(copy_buffer, entry.str, entry.length);

        const char *dep = check_deps(&h, &seen_entries);
        if (dep != NULL)
        {
            LOG_ERROR("\"%s\": \"%s\" is dependent on \"%s\"", file_path, h.name, dep);
            return FAIL;
        }

        // check for incompatibilities
        const char *inc = check_incompat(&h, &seen_entries);
        if (inc != NULL)
        {
            LOG_ERROR("\"%s\": \"%s\" is incompatible with \"%s\"", file_path, h.name, inc);
            return FAIL;
        }

        // check for slots
        const char *slot = check_slot(&h, &seen_entries, &seen_entries);
        if (slot != NULL)
        {
            LOG_ERROR("\"%s\": \"%s\" cant be an \"%s\", slot is already taken", file_path, h.name, slot);
            return FAIL;
        }

        // calling the handler

        if (h.function(copy_buffer, dest) != SUCCESS)
        {
            LOG_ERROR("\"%s\": handler \"%s\" failed to process this data: %s", file_path, h.name, copy_buffer);
            status = FAIL;
            break;
        }
        else // if the handler was successful
            (void)vec_push(&seen_entries, h.name);
    }

    vec_deinit(&seen_entries);
    vec_deinit(&filled_slots);

    return status;
}

u32 is_already_in_registry(block_resources_t *reg, block_resources *br)
{
    for (u32 i = 0; i < reg->length; i++)
        if (br->id == reg->data[i].id)
            return 1;

    return 0;
}

void call_increments(block_resources *br_ref)
{
    for (u32 j = 0; j < TOTAL_HANDLERS; j++)
        if (res_handlers[j].increment_fn) // if no increment function is
                                          // present, we skip it
            for (u32 i = 0; i < br_ref->repeat_increment.length;
                 i++) // for each entry we check if its a correct handler
                if (strcmp(res_handlers[j].name, br_ref->repeat_increment.data[i]) == 0)
                    res_handlers[j].increment_fn(br_ref);
}

void range_ids(block_resources_t *reg, block_resources *br_ref)
{
    for (u32 i = 1; i < br_ref->repeat_times; i++)
    {
        u8 skip_this_one = 0;

        for (u32 j = 0; j < br_ref->repeat_skip.length; j++) // if an index appears on out repeat skip list, we skip it
            if (br_ref->repeat_skip.data[j] == i)
            {
                skip_this_one = 1;
                break;
            }

        if (skip_this_one)
            call_increments(br_ref);

        call_increments(br_ref);
        br_ref->id++;

        FLAG_SET(br_ref->flags, RESOURCE_FLAG_RANGED,
                 1); // mark as ranged and then fix the original resource
        (void)vec_push(reg, *br_ref);
        FLAG_SET(br_ref->flags, RESOURCE_FLAG_RANGED, 0);
    }
}

void registry_fill_gaps(block_resources_t *reg, block_resources filler_entry)
{
    u32 orig_length = reg->length;

    for (u32 i = 1; i < orig_length; i++)
    {
        u32 block_prev_id = reg->data[i - 1].id;
        u32 block_cur_id = reg->data[i].id;

        if (block_prev_id + 1 != block_cur_id)
        {
            for (u32 j = block_prev_id; j < block_cur_id; j++)
            {
                if (j == 0)
                    continue;
                filler_entry.id = j;
                LOG_INFO("Adding a filler entry with an id %d", j);
                (void)vec_push(reg, filler_entry);
            }
            break;
        }
    }
}

// attempts to push a single block to the registry, might fail if something is
// wrong
u32 registry_read_block(block_registry *reg_ref, const char *file_path)
{
    block_resources br = {};
    block_resources_t *reg = &reg_ref->resources;
    br.parent_registry = reg_ref;

    if (parse_block_resources_from_file(file_path, &br) != SUCCESS)
    {
        LOG_ERROR("Failed to parse block resources from file: %s", file_path);
        return FAIL;
    }

    if (FLAG_GET(br.flags, RESOURCE_FLAG_AUTO_ID))
        br.id = reg_ref->resources.length;

    if (is_already_in_registry(reg, &br))
    {
        LOG_INFO("Found duplicate resource: %s, resource is not pushed!", file_path);
        return FAIL;
    }

    // seems like we have a valid block

    (void)vec_push(reg, br);

    // check for range thing

    if (br.repeat_times != 0)
    {
        range_ids(reg, &br);
    }

    return SUCCESS;
}

// reads all blocks in a folder and adds them to the registry
u32 registry_read_folder(block_registry *reg_ref, const char *folder_path)
{
    CHECK_PTR(reg_ref);
    DIR *directory;
    struct dirent *entry;

    char subpath[MAX_PATH_LENGTH] = {};

    directory = opendir(folder_path);

    if (directory == NULL)
    {
        LOG_ERROR("Unable to open directory: %s", folder_path);
        closedir(directory);
        return FAIL;
    }

    LOG_INFO("Reading blocks from %s folder", folder_path);

    while ((entry = readdir(directory)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        else if (entry->d_type == DT_REG) // each file will be attempted to be parsed as a block
        {
            snprintf(subpath, sizeof(subpath), "%s" SEPARATOR_STR "%s", folder_path, entry->d_name);

            if (registry_read_block(reg_ref, subpath) != SUCCESS)
            {
                LOG_ERROR("Failed to parse block resources from file: %s", subpath);
                closedir(directory);
                return FAIL;
            }
        }

        else if (entry->d_type == DT_DIR) // each folder will be attempted to be parsed as a
                                          // folder, full of blocks or other folders
        {
            snprintf(subpath, sizeof(subpath), "%s" SEPARATOR_STR "%s", folder_path, entry->d_name);

            if (registry_read_folder(reg_ref, subpath) != SUCCESS)
            {
                LOG_ERROR("Failed to parse block resources from folder: %s", subpath);
                closedir(directory);
                return FAIL;
            }
        }
    }

    return SUCCESS;
}

u32 read_block_registry(block_registry *reg_ref, const char *folder_name)
{
    CHECK_PTR(reg_ref);
    reg_ref->name = strdup(folder_name);
    reg_ref->uuid = generate_uuid();

    char reg_path[MAX_PATH_LENGTH] = {};
    snprintf(reg_path, sizeof(reg_path), FOLDER_REG SEPARATOR_STR "%s" SEPARATOR_STR "blocks", folder_name);

    LOG_INFO("Reading block registry from %s", reg_path);

    block_resources filler_entry = {
        .id = 0, .vars_sample = {{}, {}}
    };

    // push a default void block with id 0
    (void)vec_push(&reg_ref->resources, filler_entry);

    // read the registry recursively
    if (registry_read_folder(reg_ref, reg_path) != SUCCESS)
    {
        LOG_ERROR("Failed to read block registry from %s", reg_path);
        return FAIL;
    }

    // check for holes and fill them
    FLAG_SET(filler_entry.flags, RESOURCE_FLAG_IS_FILLER, 1)

    block_resources_t *reg = &reg_ref->resources;

    sort_by_id(reg);
    registry_fill_gaps(reg, filler_entry);
    sort_by_id(reg);

    // debug_print_registry(registry);

    build_atlas(reg_ref);

    if (scripting_load_scripts(reg_ref) != SUCCESS)
    {
        LOG_ERROR("Failed to load scripts for registry %s", folder_name);
        return FAIL;
    }

    return SUCCESS;
}

int __b_cmp(const void *a, const void *b)
{
    u64 id_a, id_b;
    id_a = ((block_resources *)a)->id;
    id_b = ((block_resources *)b)->id;
    return (id_a > id_b) - (id_a < id_b);
}

void sort_by_id(block_resources_t *reg)
{
    qsort(reg->data, reg->length, sizeof(*reg->data), __b_cmp);
}

void free_block_resources(block_resources *b)
{
    for (u32 i = 0; i < TOTAL_HANDLERS; i++)
        if (res_handlers[i].function(clean_token, b) != SUCCESS)
            LOG_ERROR("block id \"%lld\": handler \"%s\" failed to free block "
                      "resources",
                      b->id, res_handlers[i].name);

    if (b->id != 0 && FLAG_GET(b->flags, RESOURCE_FLAG_IS_FILLER) == 0) // ignore filler blocks and an air block, they
                                                                        // are created by engine
        free_table(b->all_fields);
}

void free_block_registry(block_registry *b_reg)
{
    LOG_INFO("Freeing block registry: %s", b_reg->name);
    for (u32 i = 0; i < b_reg->resources.length; i++)
        free_block_resources(&b_reg->resources.data[i]);
    vec_deinit(&b_reg->resources);
}

u32 read_all_registries(char *folder, vec_registries_t *dest)
{
    DIR *directory;
    struct dirent *entry;

    char pathbuf[MAX_PATH_LENGTH] = {};

    directory = opendir(folder);

    if (directory == NULL)
    {
        LOG_ERROR("Unable to open directory: %s", folder);
        closedir(directory);
        return FAIL;
    }

    while ((entry = readdir(directory)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            block_registry temp = {};

            vec_init(&temp.resources);
            temp.name = entry->d_name;

            LOG_INFO("Found directory: %s", entry->d_name);

            sprintf(pathbuf, "%s" SEPARATOR_STR "%s", folder, entry->d_name);
            LOG_INFO("Reading registry from %s", pathbuf);

            if (read_block_registry(&temp, pathbuf) != SUCCESS)
            {
                LOG_ERROR("Error reading registry from %s", pathbuf);
                closedir(directory);
                return FAIL;
            }

            (void)vec_push(dest, temp);
        }
    }
    closedir(directory);
    return SUCCESS;
}

block_registry *find_registry(vec_void_t src, char *name)
{
    for (u32 i = 0; i < src.length; i++)
        if (strcmp(((block_registry *)src.data[i])->name, name) == 0)
            return (block_registry *)src.data[i];

    return NULL;
}

void debug_print_registry(block_registry *ref)
{
    LOG_DEBUG("%s:", ref->name);

    for (u32 i = 0; i < ref->resources.length; i++)
    {
        block_resources br = ref->resources.data[i];
        LOG_DEBUG("%d: %s, %d, %x", br.id, br.texture_filename, br.override_frame, br.flags);
    }
}