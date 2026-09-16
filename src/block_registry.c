#include "include/block_registry.h"
#include "include/atlas_builder.h"
#include "include/block_properties.h"
#include "include/flags.h"
#include "include/folder_structure.h"
#include "include/scripting.h"
#include "include/uuid.h"
#include "include/vars.h"
#include "include/vars_utils.h"

#include "include/config.h"

#ifdef _WIN64
#include "dirent/include/dirent.h"
#else
#include <dirent.h>
#endif

// #include <ctype.h>
#include <stdlib.h>
#include <string.h>

u8 read_int_list(const char *str, vec_int_t *dest)
{
	assert(str);
	assert(dest);

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
	assert(str);
	assert(dest);

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

#define DECLARE_DEFALT_STR_HANDLER(name)                                                                               \
	u8 block_res_##name##_str_handler(const char *data, block_resources *dest)                                         \
	{                                                                                                                  \
		if (strcmp(data, clean_token) == 0)                                                                            \
		{                                                                                                              \
			free(dest->name);                                                                                          \
			return SUCCESS;                                                                                            \
		}                                                                                                              \
		dest->name = strdup(data);                                                                                     \
		return SUCCESS;                                                                                                \
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

	assert(dest->parent_registry);

	block_registry *b_reg = (block_registry *)dest->parent_registry;

	assert(b_reg->name);

	char sound_full_path[MAX_PATH_LENGTH] = {};

	for (u32 i = 0; i < tmp.length; i++)
	{
		sound t = {};

		PATH_SOUND_MAKE(sound_full_path, b_reg->name, tmp.data[i]);

		sound_load(&t, sound_full_path);
		(void)vec_push(&dest->sounds, t);
	}

	return SUCCESS;
}

DECLARE_DEFAULT_BYTE_FIELD_HANDLER(override_frame)

u8 block_res_data_handler(const char *data, block_resources *dest)
{
	return strcmp(data, clean_token) == 0 ? vars_free(&dest->vars_sample) : vars_parse(data, &dest->vars_sample);
}

u8 block_res_texture_handler(const char *data, block_resources *dest)
{
	if (strcmp(data, clean_token) == 0)
	{
		image_free(dest->img);
		dest->img = 0;

		SAFE_FREE(dest->texture_filename);

		return SUCCESS;
	}

	assert(dest->parent_registry);

	block_registry *b_reg = (block_registry *)dest->parent_registry;

	assert(b_reg->name);

	char texture_full_path[MAX_PATH_LENGTH] = {};
	snprintf(texture_full_path, sizeof(texture_full_path),
			 FOLDER_REG SEPARATOR_STR "%s" SEPARATOR_STR FOLDER_REG_TEX SEPARATOR_STR "%s", b_reg->name, data);

	// old way makes it a texture right away and frees ma precius pixels! we
	// need deez image pixels to make some useful work in the atlas_builder
	// later!

	// return texture_load(&dest->block_texture, texture_full_path);
	// return image_load_alt(&dest->img, &dest->block_texture,
	// texture_full_path);

	dest->img = image_load(texture_full_path);

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

// Autotiling stuff

DECLARE_DEFAULT_BYTE_FIELD_HANDLER(autotile_type)

DECLARE_DEFALT_STR_HANDLER(lua_script_filename)

DECLARE_DEFAULT_STR_VEC_HANDLER(input_names)

// "components" is a .blk entry name; it maps to the component_names vec
u8 block_res_components_vec_str_handler(const char *data, block_resources *dest)
{
	if (strcmp(data, clean_token) == 0)
	{
		int i;
		char *val;
		vec_foreach(&dest->component_names, val, i) SAFE_FREE(val);

		if (dest->component_names.data)
			vec_deinit(&dest->component_names);
		else
			vec_init(&dest->component_names);
	}
	read_str_list(data, &dest->component_names);
	return SUCCESS;
}

/* end of block resource handlers */

const static resource_entry_handler res_handlers[] = {
	// Use strict ID if you don wana break compatibility between major updates, use automatic for quick block addition
	// ENGINE DOES NOT GUARANTEES DAT BLOCK WILL HAVE THE SAME ID IF NEW ASSETS WERE ADDED!
	{
		.function = &block_res_id_handler,
		.name = "id",
		.slots = {"identity"},
	 },
	{
		.function = &block_res_automatic_id_handler,
		.name = "automatic_id",
		.slots = {"identity"},
	 },
	// Controls which file will be associated with the block for all animations and states and stuff
	{
		.function = &block_res_texture_handler,
		.name = "texture",
		.is_critical = REQUIRED,
	 },
	// Loads sounds from an array of strings
	{
		.function = &block_res_sounds_vec_handler,
		.name = "sounds",
	 },
	// Defines a set of default block variables that each new pasted instance of a block will have
	// see handler for syntax
	{
		.function = &block_res_data_handler,
		.name = "vars",
	 },
	// These are frame controllers - they will define how frames (on X-axis) will be selected for a block
	// FPS constantly changes the frame said times per second - only integers from 1 to 255
	{
		.function = &block_res_fps_handler,
		.name = "fps",
		.slots = {"frame_control"},
	 },
	// If defined, engine will look at block vars at said key and use that as a frame
	{
		.function = &block_res_anim_controller_handler,
		.name = "frame_controller",
		.deps = {"vars"},
		.slots = {"frame_control"},
	 },
	// Based on tile location, pseudo-randomly selects a frame
	{
		.function = &block_res_position_based_type_handler,
		.name = "random_frame",
		.slots = {"frame_control"},
	 },
	// Directly sets a frame from the registry. Why would anyone use this?
	{
		.function = &block_res_override_frame_handler,
		.name = "override_frame",
		.slots = {"frame_control"},
	 },
	// Based on selected block variable, flips the texture horizontally or vertiacaly or both.
	// TODO: check if it actually works
	{
		.function = &block_res_flip_controller_handler,
		.name = "flip_controller",
		.deps = {"vars"},
	 },
	// Rotates the texture around its center. Variable has to hold a u16 value in degrees to work
	{
		.function = &block_res_rotation_controller_handler,
		.name = "rotation_controller",
		.deps = {"vars"},
	 },
	// Based on said block variable, offsets the block on x-axis
	// Also used for interpolation
	{
		.function = &block_res_offset_x_controller_handler,
		.name = "offset_x_controller",
		.deps = {"vars"},
	 },
	// For y
	{
		.function = &block_res_offset_y_controller_handler,
		.name = "offset_y_controller",
		.deps = {"vars"},
	 },
	// Interpolation - takes said block variable, interprets it as a u32 tick timestamp, and linearly interpolates its
	// position between offset_x/y controlled values
	// Used in pairs with interp_takes
	{
		.function = &block_res_interp_timestamp_controller_handler,
		.name = "interp_timestamp_controller",
		.deps = {"vars"},
	 },
	// Amount of milliseconds that the thing takes to get from position dictated by offset_x/y to blocks real position
	// See jumper.blk
	{
		.function = &block_res_interp_takes_handler,
		.name = "interp_takes",
		.deps = {"vars", "interp_timestamp_controller"},
	 },
	// Similar to frame_controller, selects a type (Y-axis) frame on a texture
	{
		.function = &block_res_type_controller_handler,
		.name = "type_controller",
		.deps = {"vars"},
		.slots = {"type_control"},
	 },
	// If your texture is a single-type animation, but you dont want it to look like a long stripe of frames, you can
	// simply ignore types and make a square texture
	{
		.function = &block_res_ignore_type_handler,
		.name = "ignore_type",
		.slots = {"type_control"},
	 },
	// Selects an autotile type, according to which the engine will choose the frame for a block based on its neighbours
	// Still allows sum animations to be applied
	// Compatible with ignore-type so you can have nice 3x3 textures instead of 1x9 arrays of frames
	// 0 - no type, default
	// 1 - 3x3 tileset, 4 neighbours, lines default to center tile
	// 2 - 4x4, 4 neighbours, full coverage on all cases
	// 3 - 47 autotile scheme, 8 neighbour scheme
	// {
	//  .function = &block_res_autotile_update_key_handler,
	//  .name = "autotile_update_key",
	//  .deps = {"vars"},
	//  },
	// {
	//  .function = &block_res_autotile_cache_key_handler,
	//  .name = "autotile_cache_key",
	//  .deps = {"vars"},
	//  },
	{
		.function = &block_res_autotile_type_handler,
		.name = "autotile_type",
		//  .deps = {"autotile_update_key", "autotile_cache_key"},
		.slots = {"frame_control"},
	 },

	// Associates said block with a script file - used for inputs, tick events, etc
	{
		.function = &block_res_lua_script_filename_str_handler,
		.name = "script",
	 },
	// Used to accept ticks and other block inputs. Handlers may be registered
	// by a block script or by components, so no dependency on "script" is enforced.
	{
		.function = &block_res_input_names_vec_str_handler,
		.name = "inputs",
	 },
	// List of component scripts attached to the block. Each component runs at
	// registry load and can alter the block resource (vars, interpolation, APIs).
	{
		.function = &block_res_components_vec_str_handler,
		.name = "components",
	 },
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
		const char *slot = check_slot(&h, &seen_entries, &filled_slots);
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

static void put_fillers_until(block_resources_t *reg, block_resources entry, u32 block_start, u32 block_end)
{
	for (u32 j = block_start; j < block_end; j++)
	{
		if (j == 0)
			continue;
		entry.id = j;
		LOG_INFO("Adding a filler entry with an id %d", j);
		(void)vec_push(reg, entry);
	}
}

void registry_fill_gaps(block_resources_t *reg, block_resources filler_entry)
{
	u32 orig_length = reg->length;

	// start from 1 because block 0 is void
	for (u32 i = 1; i < orig_length; i++)
	{
		u32 block_prev_id = reg->data[i - 1].id;
		u32 block_cur_id = reg->data[i].id;

		if (block_prev_id + 1 != block_cur_id)
		{
			put_fillers_until(reg, filler_entry, block_prev_id, block_cur_id);
			// note: previous code stopped right here, exiting the loop early. This means that before
			// this code only handled 1 id hole in the registry, leaving anything after this intact.
			// that would mean that something might break when this time, we scan for every hole.
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

	return SUCCESS;
}

char *block_source_name(block_resources *b)
{
	if (!b || !b->all_fields)
		return NULL;

	blob src = get_entry(b->all_fields, blobify("source_filename"));
	if (!src.ptr)
		return NULL;

	const char *base = src.str;
	const char *last_slash = NULL;
	for (const char *p = base; *p; p++)
		if (*p == '/' || *p == '\\')
			last_slash = p;
	if (last_slash)
		base = last_slash + 1;

	const char *dot = strrchr(base, '.');
	u32 len = (u32)(dot ? (size_t)(dot - base) : strlen(base));

	char *name = malloc(len + 1);
	assert(name != NULL);
	memcpy(name, base, len);
	name[len] = '\0';
	return name;
}

u64 block_registry_find_id_by_name(block_registry *reg, const char *name)
{
	assert(reg != NULL);
	if (!name)
		return FAIL;

	for (u32 i = 0; i < reg->resources.length; i++)
	{
		char *tmp = block_source_name(&reg->resources.data[i]);
		if (!tmp)
			continue;
		i32 eq = strcmp(tmp, name);
		SAFE_FREE(tmp);
		if (eq == 0)
			return reg->resources.data[i].id;
	}

	return FAIL;
}

// reads all blocks in a folder and adds them to the registry
u32 registry_read_folder(block_registry *reg_ref, const char *folder_path)
{
	assert(reg_ref);
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
	assert(reg_ref);
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
			LOG_ERROR("block id \"%lld\": handler \"%s\" failed to free block resources", b->id, res_handlers[i].name);

	// ignore filler blocks and an air block, they re created by the engine
	if (b->id != 0 && FLAG_GET(b->flags, RESOURCE_FLAG_IS_FILLER) == 0)
		free_table(b->all_fields);

	// they could still have scripts attached though, which may sound weird...

	if (b->lua_script_blob)
	{
		free(b->lua_script_blob);
		b->lua_script_blob = NULL;
		b->lua_script_blob_size = 0;
	}

	if (b->lua_script_filename)
	{
		free(b->lua_script_filename);
		b->lua_script_filename = NULL;
	}
}

void free_block_registry(block_registry *b_reg)
{
	LOG_INFO("Freeing block registry: %s", b_reg->name);
	for (u32 i = 0; i < b_reg->resources.length; i++)
		free_block_resources(&b_reg->resources.data[i]);
	vec_deinit(&b_reg->resources);

	for (u32 i = 0; i < b_reg->component_blobs.length; i++)
	{
		SAFE_FREE(b_reg->component_blobs.data[i].name);
		SAFE_FREE(b_reg->component_blobs.data[i].blob);
	}
	vec_deinit(&b_reg->component_blobs);
}

component_blob_entry *registry_find_component_blob(block_registry *reg, const char *name)
{
	assert(reg);
	assert(name);

	for (u32 i = 0; i < reg->component_blobs.length; i++)
		if (strcmp(reg->component_blobs.data[i].name, name) == 0)
			return &reg->component_blobs.data[i];

	return NULL;
}

void rebuild_vars_offsets(block_resources *res)
{
	for (i32 i = 0; i < (i32)(sizeof(res->vars_offsets) / sizeof(res->vars_offsets[0])); i++)
		res->vars_offsets[i] = FAIL;

	if (res->vars_sample.ptr && res->vars_sample.size > 0)
	{
		u32 pos = 0;
		while (pos + 1 < res->vars_sample.size)
		{
			u8 letter = res->vars_sample.ptr[pos];
			u8 size = res->vars_sample.ptr[pos + 1];
			res->vars_offsets[letter] = (i32)pos;
			pos += (u32)size + 2;
		}
	}
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