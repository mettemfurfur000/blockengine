#include "../include/block_registry.h"
#include "../include/uuid.h"

#define EVAL_SIZE_MATCH(v, t) v >> (sizeof(t) * 8 - 8)
#define RETURN_MATCHING_SIZE(v, t) \
	if (EVAL_SIZE_MATCH(v, t))     \
		return sizeof(t);
// some kind of runtime type detection... returns amout of bytes, occupied by a number
u8 length(u64 value)
{
	RETURN_MATCHING_SIZE(value, long long)
	RETURN_MATCHING_SIZE(value, long)
	RETURN_MATCHING_SIZE(value, int)
	RETURN_MATCHING_SIZE(value, short)

	return 1;
}

void strip_digit(u8 *dest, u64 value, u32 actual_length)
{
	u8 *value_p = (u8 *)&value;
	for (u32 i = 0; i < actual_length; i++)
		dest[i] = value_p[i];
}

u32 str_to_enum(char *type_str)
{
	if (!strcmp(type_str, "digit"))
		return T_DIGIT;
	if (!strcmp(type_str, "long"))
		return T_LONG;
	if (!strcmp(type_str, "int"))
		return T_INT;
	if (!strcmp(type_str, "short"))
		return T_SHORT;
	if (!strcmp(type_str, "byte"))
		return T_BYTE;
	if (!strcmp(type_str, "string"))
		return T_STRING;
	return T_UNKNOWN;
}

u32 get_length_to_alloc(u64 value, u32 type)
{
	switch (type)
	{
	case T_DIGIT:
		return length(value);
	case T_LONG:
		return sizeof(u64);
	case T_INT:
		return sizeof(u32);
	case T_SHORT:
		return sizeof(u16);
	case T_BYTE:
		return sizeof(u8);
	default:
		return length(value);
	}
}

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

u8 make_block_data_from_string(const char *str_to_cpy, blob *b)
{
	CHECK_PTR(str_to_cpy)
	CHECK_PTR(b)

	char character;
	u8 data_buffer[256] = {0};
	u8 data_to_load[256] = {0};

	data_to_load[0] = 0;   // size byte
	u32 data_iterator = 0; // starts with 1 because of size u8 at start

	u64 value;
	u32 value_length;

	char *str = strdup(str_to_cpy);
	strcpy(str, str_to_cpy);

	char *token = strtok(str, " \n"); // consume start token
	if (strcmp(token, "{") != 0)
		goto block_data_exit;

	while (1)
	{
		token = strtok(NULL, " \n"); // type, or end token..

		if (!token)
			goto block_data_exit;

		if (strcmp(token, "}") == 0)
			goto block_data_exit;

		u32 type = str_to_enum(token);

		token = strtok(NULL, " \n"); // character

		if (!token)
			goto block_data_exit;

		character = token[0]; // copy just 1 character

		memset(data_buffer, 0, 256);
		value = 0;
		value_length = 0;

		// special cases for strings or unknown types
		if (type == T_STRING || type == T_UNKNOWN)
		{
			token = strtok_take_whole_line();
			value_length = strlen(token);
			memcpy(data_buffer, token, value_length + 1);
		}
		else
		{
			token = strtok(NULL, " =\n"); // value

			if (!token)
				goto block_data_exit;
			value = atoll(token);

			value_length = get_length_to_alloc(value, type);

			make_endianless((u8 *)&value, value_length);
			memcpy(data_buffer, &value, value_length);
			strip_digit(data_buffer, value, value_length);
		}

		data_to_load[data_iterator] = character;
		data_iterator++;
		data_to_load[data_iterator] = value_length;
		data_iterator++;
		for (u32 i = 0; i < value_length; i++, data_iterator++)
			data_to_load[data_iterator] = data_buffer[i];
	}
block_data_exit:
	if (data_iterator == 0) // ok, no data
		return SUCCESS;

	// data_to_load[0] = data_iterator - 1; // writing size byte
	b->size = data_iterator;

	b->ptr = (u8 *)malloc(data_iterator);

	memcpy(b->ptr, data_to_load, data_iterator);

	free(str);

	return SUCCESS;
}

/*
all block resources handlers must return SUCCESS if they handled data

if data contains a special clean token, handler will try to free/clear data from dest resource

also handlers must return FAIL if they cant handle data or if data is invalid
*/

#define DECLARE_DEFAULT_FLAG_HANDLER(name, flag)                                              \
	u8 block_res_##name##_handler(const char *data, block_resources *dest)                    \
	{                                                                                         \
		FLAG_CONFIGURE(dest->flags, flag, atoi(data) ? 1 : 0, strcmp(data, clean_token) == 0) \
		return SUCCESS;                                                                       \
	}

#define DECLARE_DEFAULT_CHAR_FIELD_HANDLER(name)                           \
	u8 block_res_##name##_handler(const char *data, block_resources *dest) \
	{                                                                      \
		if (strcmp(data, clean_token) == 0)                                \
		{                                                                  \
			dest->name = 0;                                                \
			return SUCCESS;                                                \
		}                                                                  \
		dest->name = data[0];                                              \
		return SUCCESS;                                                    \
	}

#define DECLARE_DEFAULT_BYTE_FIELD_HANDLER(name)                           \
	u8 block_res_##name##_handler(const char *data, block_resources *dest) \
	{                                                                      \
		if (strcmp(data, clean_token) == 0)                                \
		{                                                                  \
			dest->name = 0;                                                \
			return SUCCESS;                                                \
		}                                                                  \
		dest->name = atoi(data);                                           \
		return SUCCESS;                                                    \
	}

#define DECLARE_DEFAULT_LONG_FIELD_HANDLER(name)                           \
	u8 block_res_##name##_handler(const char *data, block_resources *dest) \
	{                                                                      \
		if (strcmp(data, clean_token) == 0)                                \
		{                                                                  \
			dest->name = 0;                                                \
			return SUCCESS;                                                \
		}                                                                  \
		dest->name = atol(data);                                           \
		return SUCCESS;                                                    \
	}

#define DECLARE_DEFAULT_INCREMENTOR(name)                      \
	void block_res_##name##_incrementor(block_resources *dest) \
	{                                                          \
		dest->name++;                                          \
	}

#define DECLADE_DEFAUALT_STR_VEC_HANDLER(name)                                     \
	u8 block_res_##name##_vec_str_handler(const char *data, block_resources *dest) \
	{                                                                              \
		if (strcmp(data, clean_token) == 0)                                        \
		{                                                                          \
			int i;                                                                 \
			char *val;                                                             \
			vec_foreach(&dest->name, val, i)                                       \
			{                                                                      \
				SAFE_FREE(val);                                                    \
			}                                                                      \
			if (dest->name.data)                                                   \
				vec_deinit(&dest->name);                                           \
			else                                                                   \
				vec_init(&dest->name);                                             \
		}                                                                          \
		read_str_list(data, &dest->name);                                          \
		return SUCCESS;                                                            \
	}

const static char clean_token[] = "??clean??";

DECLARE_DEFAULT_LONG_FIELD_HANDLER(id)
DECLARE_DEFAULT_LONG_FIELD_HANDLER(ranged_id)
// DECLARE_DEFAULT_LONG_FIELD_HANDLER(id_range_skip)

u8 block_res_id_range_skip_handler(const char *data, block_resources *dest)
{
	if (strcmp(data, clean_token) == 0)
	{
		if (dest->id_range_skip.data)
			vec_deinit(&dest->id_range_skip);
		else
			vec_init(&dest->id_range_skip);

		return SUCCESS;
	}

	read_int_list(data, &dest->id_range_skip);

	return SUCCESS;
}

DECLADE_DEFAUALT_STR_VEC_HANDLER(id_range_increment)
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

	for (u32 i = 0; i < tmp.length; i++)
	{
		sound t = {};
		sound_load(&t, tmp.data[i]);
		(void)vec_push(&dest->sounds, t);
	}

	return SUCCESS;
}

DECLARE_DEFAULT_BYTE_FIELD_HANDLER(override_frame)
DECLARE_DEFAULT_INCREMENTOR(override_frame)

u8 block_res_data_handler(const char *data, block_resources *dest)
{
	return strcmp(data, clean_token) == 0
			   ? var_delete_all(&dest->vars)
			   : make_block_data_from_string(data, &dest->vars);
}

u8 block_res_texture_handler(const char *data, block_resources *dest)
{
	if (strcmp(data, clean_token) == 0)
	{
		free_texture(&dest->block_texture);
		return SUCCESS;
	}

	CHECK_PTR(dest->parent_registry)

	block_registry *b_reg = (block_registry *)dest->parent_registry;

	CHECK_PTR(b_reg->name)

	char texture_full_path[256] = {};
	snprintf(texture_full_path, sizeof(texture_full_path), REGISTRIES_FOLDER SEPARATOR_STR "%s" SEPARATOR_STR REGISTRY_TEXTURES_FOLDER SEPARATOR_STR "%s", b_reg->name, data);

	return texture_load(&dest->block_texture, texture_full_path);
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

DECLARE_DEFAULT_FLAG_HANDLER(ignore_type, B_RES_FLAG_IGNORE_TYPE)
DECLARE_DEFAULT_FLAG_HANDLER(position_based_type, B_RES_FLAG_RANDOM_POS)

DECLARE_DEFAULT_CHAR_FIELD_HANDLER(anim_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(type_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(flip_controller)
DECLARE_DEFAULT_CHAR_FIELD_HANDLER(rotation_controller)

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

/* end of block resource handlers */

const static resource_entry_handler res_handlers[] = {
	{NULL, &block_res_id_handler, "id", REQUIRED, {}, {}},
	{NULL, &block_res_texture_handler, "texture", REQUIRED, {}, {}},
	{NULL, &block_res_sounds_vec_handler, "sounds", NOT_REQUIRED, {}, {}},
	// dangerous: will replicate the same block multiple times until it reaches the said id
	// useful if you just want to have a bunch of blocks with the same texture and locked type
	{NULL, &block_res_ranged_id_handler, "id_ranged_to", NOT_REQUIRED, {}, {}},
	// if id accurs on this list while ranging, all increments will pass but no blocks will be pasted
	{NULL, &block_res_id_range_skip_handler, "id_range_skip", NOT_REQUIRED, {"id_ranged_to"}, {}},
	// this controls what variables shoud be incremented as id_range progresses forward
	// triggers them and passes id of current ranged block as a string
	{NULL, &block_res_id_range_increment_vec_str_handler, "id_range_increment", NOT_REQUIRED, {"id_ranged_to"}, {}},

	{NULL, &block_res_data_handler, "data", NOT_REQUIRED, {}, {}},

	// graphics-related stuff
	// frames
	{NULL, &block_res_fps_handler, "fps", NOT_REQUIRED, {}, {"frame_controller", "override_frame"}},
	{NULL, &block_res_anim_controller_handler, "frame_controller", NOT_REQUIRED, {"data"}, {"fps", "override_frame"}},											 // directly controls the frame of a texture, so it cant used with fps
	{NULL, &block_res_position_based_type_handler, "random_frame", NOT_REQUIRED, {}, {"frame_controller", "override_frame"}},									 // randomizes frame of a block based on its location
	{&block_res_override_frame_incrementor, &block_res_override_frame_handler, "override_frame", NOT_REQUIRED, {}, {"fps", "frame_controller", "random_frame"}}, // directly sets its type from block resources - solid as rock
	// flips, intested
	{NULL, &block_res_flip_controller_handler, "flip_controller", NOT_REQUIRED, {"data"}, {}},
	// untested
	{NULL, &block_res_rotation_controller_handler, "rotation_controller", NOT_REQUIRED, {"data"}, {}},
	// block type things
	{NULL, &block_res_type_controller_handler, "type_controller", NOT_REQUIRED, {"data"}, {"ignore_type"}}, // reads its type from a block data - can be controlled with scripts
	{NULL, &block_res_ignore_type_handler, "ignore_type", NOT_REQUIRED, {}, {"type_controller"}},			// ignores type and treats each 16x16 square on a texture as a frame

	{NULL, &block_res_lua_script_handler, "script", NOT_REQUIRED, {}, {}}

};

const u32 TOTAL_HANDLERS = sizeof(res_handlers) / sizeof(*res_handlers);

u32 parse_block_resources_from_file(char *file_path, block_resources *dest)
{
	hash_node **properties = alloc_table();
	u32 status = SUCCESS;

	if (load_properties(file_path, properties) == FAIL)
	{
		LOG_ERROR("Error loading properties: %s", file_path);

		free_table(properties);
		return FAIL;
	}

	put_entry(properties, blobify("source_filename"), blobify(file_path));

	// also include a full file in block resources, for whatever reason
	dest->all_fields = properties;

	blob entry = {};

	vec_str_t seen_entries;
	vec_init(&seen_entries);

	for (u32 i = 0; i < TOTAL_HANDLERS; i++)
	{
		entry = get_entry(properties, blobify(res_handlers[i].name));

		if (!entry.ptr)
		{
			if (res_handlers[i].is_critical)
			{
				LOG_ERROR("\"%s\" : No \"%s\" entry", file_path, res_handlers[i].name);
				status = FAIL;
				break;
			}
			continue;
		}
		// fix string to be null terminated
		// this line of code caused alot of headaches for me, i hate myself
		entry.str[entry.length - 1] = '\0';
		// check for dependencies
		for (u32 j = 0; j < sizeof(res_handlers[i].dependencies) / sizeof(void *); j++)
		{
			char *dep = res_handlers[i].dependencies[j];
			if (!dep)
				continue;

			if (seen_entries.length == 0)
			{
				LOG_ERROR("\"%s\": \"%s\" is dependent on \"%s\", but seen entries array is empty", file_path, res_handlers[i].name, dep);
				status = FAIL;
				break;
			}
			u32 k = 0;
			vec_find(&seen_entries, dep, k);
			if (k == -1)
			{
				LOG_ERROR("\"%s\": \"%s\" is dependent on \"%s\"", file_path, res_handlers[i].name, dep);
				status = FAIL;
				break;
			}

			LOG_DEBUG("\"%s\": Dependency \"%s\" is found for \"%s\"", file_path, dep, res_handlers[i].name);
		}

		// check for incompatibilities
		// basicly same as incompatibilities but instead of requiring certain fields it skips them
		for (u32 j = 0; j < sizeof(res_handlers[i].incompabilities) / sizeof(void *); j++)
		{
			char *inc = res_handlers[i].incompabilities[j];
			if (!inc)
				continue;

			if (seen_entries.length == 0)
				continue;

			u32 k = 0;
			vec_find(&seen_entries, inc, k);
			if (k == -1)
				continue;

			LOG_ERROR("\"%s\": \"%s\" is incompatible with \"%s\"", file_path, res_handlers[i].name, inc);
			status = FAIL;
			break;
		}

		// calling the handler

		if (res_handlers[i].function(entry.str, dest) != SUCCESS)
		{
			LOG_ERROR("\"%s\": handler \"%s\" failed to process this data: %s", file_path, res_handlers[i].name, entry.str);
			status = FAIL;
			break;
		}
		else // if the handler was successful
			(void)vec_push(&seen_entries, res_handlers[i].name);
	}

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
		if (res_handlers[j].increment_fn)								// if no increment function is present, we skip it
			for (u32 i = 0; i < br_ref->id_range_increment.length; i++) // for each entry we check if its a correct handler
				if (strcmp(res_handlers[j].name, br_ref->id_range_increment.data[i]) == 0)
					res_handlers[j].increment_fn(br_ref);
}

void range_ids(block_resources_t *reg, block_resources *br_ref)
{
	for (u32 i = 1; i <= br_ref->ranged_id; i++)
	{
		u8 skip_this_one = 0;

		for (u32 j = 0; j < br_ref->id_range_skip.length; j++)
			if (br_ref->id_range_skip.data[j] == i)
			{
				skip_this_one = 1;
				break;
			}

		if (skip_this_one)
			call_increments(br_ref);

		call_increments(br_ref);
		br_ref->id++;
		(void)vec_push(reg, *br_ref);
	}
}

u32 read_block_registry(const char *name, block_registry *registry)
{
	CHECK_PTR(registry);
	registry->name = name;
	registry->uuid = generate_uuid();

	block_resources_t *reg = &registry->resources;
	DIR *directory;
	struct dirent *entry;

	char path[64] = {};
	snprintf(path, sizeof(path), REGISTRIES_FOLDER SEPARATOR_STR "%s" SEPARATOR_STR "blocks", name);
	directory = opendir(path);

	if (directory == NULL)
	{
		LOG_ERROR("Unable to open directory: %s", path);
		closedir(directory);
		return FAIL;
	}

	LOG_INFO("Reading block registry from %s", path);

	// push a default void block with id 0
	block_resources filler_entry = {.id = 0, .vars = {{}, {}}, .type_controller = 0, .frames_per_second = 0, .anim_controller = 0};
	(void)vec_push(reg, filler_entry);

	while ((entry = readdir(directory)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (entry->d_type == DT_REG)
		{
			block_resources br = {.type_controller = 0, .frames_per_second = 0, .anim_controller = 0, .parent_registry = registry};
			char file_to_parse[468] = {};
			snprintf(file_to_parse, sizeof(file_to_parse), "%s" SEPARATOR_STR "%s", path, entry->d_name);

			if (parse_block_resources_from_file(file_to_parse, &br) == FAIL)
			{
				LOG_ERROR("Failed to parse block resources from file: %s", file_to_parse);
				free_block_resources(&br);
				continue;
			}

			if (is_already_in_registry(reg, &br))
			{
				LOG_INFO("Found duplicate resource: %s, resource is not pushed!", file_to_parse);
				continue;
			}

			(void)vec_push(reg, br);

			// check for range thing

			if (br.ranged_id != 0)
			{
				range_ids(reg, &br);
			}
		}
	}

	FLAG_SET(filler_entry.flags, B_RES_FLAG_IS_FILLER, 1)

	sort_by_id(registry);

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

	sort_by_id(registry);

	debug_print_registry(registry);

	closedir(directory);

	return SUCCESS;
}

int __b_cmp(const void *a, const void *b)
{
	u64 id_a, id_b;
	id_a = ((block_resources *)a)->id;
	id_b = ((block_resources *)b)->id;
	return (id_a > id_b) - (id_a < id_b);
}

void sort_by_id(block_registry *b_reg)
{
	qsort(b_reg->resources.data, b_reg->resources.length, sizeof(*b_reg->resources.data), __b_cmp);
}

void free_block_resources(block_resources *b)
{
	for (u32 i = 0; i < TOTAL_HANDLERS; i++)
		if (res_handlers[i].function(clean_token, b) != SUCCESS)
			LOG_ERROR("block id \"%lld\": handler \"%s\" failed to free block resources", b->id, res_handlers[i].name);

	if (b->id != 0 && FLAG_GET(b->flags, B_RES_FLAG_IS_FILLER) == 0) // ignore filler blocks and an air block, they are created by engine
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

			if (read_block_registry(pathbuf, &temp) == FAIL)
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
		LOG_DEBUG("%d: %s, %d, %x", br.id, br.block_texture.filename, br.override_frame, br.flags);
	}
}