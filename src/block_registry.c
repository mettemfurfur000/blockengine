#include "../include/block_registry.h"

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

const static char clean_token[] = "??clean??";

u8 block_res_id_handler(const char *data, block_resources *dest)
{
	dest->id = strcmp(data, clean_token) == 0 ? 0 : atoi(data);

	return SUCCESS;
}

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
	snprintf(texture_full_path, sizeof(texture_full_path), REGISTRIES_FOLDER "/%s/" REGISTRY_TEXTURES_FOLDER "/%s", b_reg->name, data);

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
	{&block_res_id_handler, "id", REQUIRED},
	{&block_res_texture_handler, "texture", REQUIRED},

	{&block_res_data_handler, "data", NOT_REQUIRED},

	//{&block_res_anim_toggle_handler, "is_animated", NOT_REQUIRED, {}, {"frame_controller"}}, //if fps is defined, it is animated
	{&block_res_fps_handler, "fps", NOT_REQUIRED, {}, {"frame_controller"}},

	{&block_res_type_controller_handler, "type_controller", NOT_REQUIRED, {"data"}, {}},
	{&block_res_anim_controller_handler, "frame_controller", NOT_REQUIRED, {"data"}, {"fps"}}, // directly controls the frame of a texture, so it cant used with fps
	{&block_res_flip_controller_handler, "flip_controller", NOT_REQUIRED, {"data"}, {}},
	{&block_res_rotation_controller_handler, "rotation_controller", NOT_REQUIRED, {"data"}, {}},

	{&block_res_ignore_type_handler, "ignore_type", NOT_REQUIRED, {}, {}},
	{&block_res_position_based_type_handler, "random_type", NOT_REQUIRED, {}, {}},

	{&block_res_lua_script_handler, "script", NOT_REQUIRED, {}, {}}

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

		if (res_handlers[i].function(entry.str, dest) != SUCCESS) // handler is beink called here
		{
			LOG_ERROR("\"%s\": handler \"%s\" failed to process this data: %s", file_path, res_handlers[i].name, entry.str);
			status = FAIL;
			break;
		}
		else
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

u32 read_block_registry(const char *name, block_registry *registry)
{
	CHECK_PTR(registry);
	registry->name = name;

	block_resources_t *reg = &registry->resources;
	DIR *directory;
	struct dirent *entry;

	char path[64] = {};
	snprintf(path, sizeof(path), REGISTRIES_FOLDER "/%s/blocks", name);
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
			snprintf(file_to_parse, sizeof(file_to_parse), "%s/%s", path, entry->d_name);

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
			for (u32 j = block_prev_id; j < block_cur_id - 1; j++)
			{
				filler_entry.id = j;
				LOG_INFO("Adding a filler entry with an id %d", j);
				(void)vec_push(reg, filler_entry);
			}
			break;
		}
	}

	sort_by_id(registry);

	closedir(directory);

	return SUCCESS;
}

int __b_cmp(const void *a, const void *b)
{
	return ((block_resources *)a)->id > ((block_resources *)b)->id;
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

			sprintf(pathbuf, "%s%c%s", folder, SEPARATOR, entry->d_name);
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

block_registry *find_registry(vec_registries_t src, char *name)
{
	for (u32 i = 0; i < src.length; i++)
		if (strcmp(src.data[i].name, name) == 0)
			return &src.data[i];

	return NULL;
}