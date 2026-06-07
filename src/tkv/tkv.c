#include "include/tkv.h"
#include "include/tokenizer.h"

#include <limits.h>
#include <lua.h>
#include <stdint.h>
#include <stdlib.h>

// Check if character is allowed in a TKV key (alphanumeric, underscore, or null terminator)
bool util_is_valid_char(char val)
{
	return (val >= 'a' && val <= 'z') ||  // lowercase letter
		   (val >= 'A' && val <= 'Z') ||  // uppercase letter
		   (val >= '0' && val <= '9') ||  // digit
		   (val == '_') ||                // underscore
		   (val == 0);                    // null terminator
}

/*==============================================================================
  CHARACTER COMPRESSION
  
  Compresses a single ASCII character to 6-bit value for key storage.
  Maps characters as follows:
    0      -> '\0' (end-of-string marker)
    1-26   -> 'a'-'z' (lowercase letters)
    27-52  -> 'A'-'Z' (uppercase letters)
    53-62  -> '0'-'9' (digits)
    63     -> '_' (underscore)
    -1     -> unknown character (error)
===============================================================================*/
i8 util_compress_char(char val)
{
	const u8 alpha_count = ('z' - 'a') + 1;  // 26 letters

	if (val == 0)
		return 0;  // null terminator

	if (val >= 'a' && val <= 'z')
		return 1 + val - 'a';  // 1-26 for lowercase

	if (val >= 'A' && val <= 'Z')
		return 1 + alpha_count + val - 'A';  // 27-52 for uppercase

	if (val >= '0' && val <= '9')
		return 1 + 2 * alpha_count + val - '0';  // 53-62 for digits

	if (val == '_')
		return 63;  // 63 for underscore

	return -1;  // unknown character
}

/*==============================================================================
  CHARACTER DECOMPRESSION
  
  Reverses util_compress_char - converts 6-bit value back to ASCII character.
  Inverse mapping of the compression function above.
===============================================================================*/
char util_decompress_char(i8 compressed_val)
{
	if (compressed_val == 0)
		return '\0';  // null terminator

	// 1-26 map to 'a'-'z'
	if (compressed_val >= 1 && compressed_val <= 26)
		return 'a' + compressed_val - 1;

	// 27-52 map to 'A'-'Z'
	if (compressed_val >= 27 && compressed_val <= 52)
		return 'A' + compressed_val - 27;

	// 53-62 map to '0'-'9'
	if (compressed_val >= 53 && compressed_val <= 62)
		return '0' + compressed_val - 53;

	// 63 maps to '_'
	if (compressed_val == 63)
		return '_';

	assert(0 && "Invalid compressed character value (0-63 expected)");
	return -1;
}

// Validate that a string is a valid TKV variable name
bool tkv_is_valid_key(const char *input)
{
	u64 len = strlen(input);

	// Key must be 1-10 characters
	if (len > TKV_KEY_LEN_MAX || len == 0)
		return false;

	// All characters must be valid
	for (u32 i = 0; i < len; i++)
		if (!util_is_valid_char(input[i]))
			return false;

	return true;
}

/*==============================================================================
  KEY ENCODING
  
  Encodes a string into a compressed tkv_key structure using 6-bit encoding.
  Each character is compressed to 6 bits, and up to 10 characters fit in 60 bits.
  The 4-bit size field stores the actual string length.
  
  Returns TKV_INVALID_KEY if the string is invalid (empty, too long, or
  contains invalid characters).
===============================================================================*/
tkv_key tkv_make_key(const char *input)
{
	assert(input);

	u64 len = strlen(input);

	if (len > TKV_KEY_LEN_MAX || len == 0)
		return TKV_INVALID_KEY;

	u64 compressed_payload = 0;

	// Compress each character and pack into 6-bit fields
	for (u64 i = 0; i < len; i++)
	{
		i8 compressed_char = util_compress_char(input[i]);
		if (compressed_char < 0)
			return TKV_INVALID_KEY;
		compressed_payload = (compressed_payload << 6) | compressed_char;
	}

	return (tkv_key){.payload = compressed_payload, .size = len};
}

/*==============================================================================
  KEY DECODING
  
  Reverses tkv_make_key - converts a compressed tkv_key back to a string.
  Extracts the size from the key's size field, then decompresses each 6-bit
  character and writes them in reverse order (since the payload was left-shifted
  during compression).
  
  NOTE: The caller must ensure 'out' has at least TKV_KEY_LEN_MAX + 1 bytes.
===============================================================================*/
void tkv_key_to_str(const tkv_key key, char *out)
{
	u64 compressed_payload = key.payload;
	const u8 key_length = key.size;
	const u8 last_index = key_length - 1;

	// Characters were left-shifted during encoding, so extract in reverse
	for (u64 i = 0; i < key_length; i++)
	{
		u8 compressed_char = compressed_payload & 0x3f;  // Extract lowest 6 bits
		out[last_index - i] = util_decompress_char(compressed_char);
		compressed_payload >>= 6;  // Shift to next character
	}
}

/*==============================================================================
  TKV OBJECT ACCESSORS
  
  These functions extract header information and elements from a serialized
  TKV object in binary format. The layout is:
  
  [Header: u16 length, u32 size]
  [Keys: tkv_key[length]]
  [Metadata: tkv_value_meta[length]]
  [Values: u8[...]]
===============================================================================*/

u16 tkv_object_get_length(tkv_object object)
{
	return *(u16 *)(object + 0);
}

u32 tkv_object_get_size(tkv_object object)
{
	return *(u32 *)(object + sizeof(u16));
}

tkv_key tkv_object_get_key(tkv_object object, u16 index)
{
	tkv_key *keys = (tkv_key *)(object + sizeof(u16) + sizeof(u32));
	return keys[index];
}

tkv_value tkv_get_value(tkv_object object, const char *key_str)
{
	tkv_key key = tkv_make_key(key_str);
	tkv_value result = {};

	if (!key.whole)
		return result;

	u16 keys_total = tkv_object_get_length(object);

	tkv_value_meta meta = {};
	i32 meta_index = -1;

	for (u16 i = 0; i < keys_total; i++)
	{
		tkv_key k = tkv_object_get_key(object, i);

		if (k.size != key.size)
			continue;

		if (k.payload != key.payload)
			continue;

		meta_index = i;
		break;
	}

	if (meta_index == -1)
		return (tkv_value){.meta.whole = UINT_MAX}; // Key not found

	tkv_value_meta *metas = (tkv_value_meta *)(object + sizeof(u16) + sizeof(u32) + sizeof(tkv_key) * keys_total);
	meta = metas[meta_index];

	result.meta = meta;
	result.tkv_meta_index = meta_index;
	result.ptr = (object + meta.tkv_value_offset);

	return result;
}

// This is a helper for getting nested values with a single function call, without manually traversing each level.
// It will be useful for the network sync system, where we want to get a value from a nested TKV with a single path
// string.
tkv_value tkv_traverse_get_value(tkv_object object, const char *path)
{
	tkv_value result = {};

	char path_copy[256];

	assert(strlen(path) < sizeof(path_copy)); // just to be safe, we can remove this later if needed

	strncpy(path_copy, path, sizeof(path_copy));
	path_copy[sizeof(path_copy) - 1] = '\0';

	char *token = strtok(path_copy, ".");
	tkv_object current_object = object;

	while (token)
	{
		result = tkv_get_value(current_object, token);
		if (result.meta.whole == UINT_MAX)
			return (tkv_value){.meta.whole = UINT_MAX}; // Key not found

		if (result.meta.tkv_value_type != TKV_VALUE_TKV)
			return result; // Found the value

		current_object = tkv_value_to_tkv(result);
		token = strtok(NULL, ".");
	}

	return result;
}

tkv_object tkv_value_get_root(tkv_value value)
{
	return value.ptr - value.meta.tkv_value_offset;
}

bool tkv_value_to_bool(tkv_value value)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_BOOL);
	return *(bool *)value.ptr;
}

i64 tkv_value_to_i64(tkv_value value)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_I64);
	return *(i64 *)value.ptr;
}

f64 tkv_value_to_f64(tkv_value value)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_F64);
	return *(f64 *)value.ptr;
}

char *tkv_value_to_str(tkv_value value)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_STR);
	return value.ptr;
}

tkv_array tkv_value_to_arr(tkv_value value)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_ARR);
	return (tkv_array){
		.element_size = *((u16 *)(value.ptr + 0)),			 //
		.array_length = *((u16 *)(value.ptr + sizeof(u16))), //
		.bytes = value.ptr + 2 * sizeof(u16)				 //
	};
}

tkv_object tkv_value_to_tkv(tkv_value value)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_TKV);
	return *(tkv_object *)value.ptr;
}

void tkv_value_set_bool(tkv_value value, bool new_val)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_BOOL);
	assert(value.meta.tkv_value_state != TKV_STATE_CONST);
	*(bool *)value.ptr = new_val;
}

void tkv_value_set_i64(tkv_value value, i64 new_val)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_I64);
	assert(value.meta.tkv_value_state != TKV_STATE_CONST);
	*(i64 *)value.ptr = new_val;
}

void tkv_value_set_f64(tkv_value value, f64 new_val)
{
	assert(value.meta.tkv_value_type == TKV_VALUE_F64);
	assert(value.meta.tkv_value_state != TKV_STATE_CONST);
	*(f64 *)value.ptr = new_val;
}

void tkv_value_set_changed(tkv_value value)
{
	if (value.meta.tkv_value_state != TKV_STATE_NETWORKABLE)
		return;

	// tkv_object object = tkv_value_get_root(value);

	// u16 keys_total = tkv_object_get_length(object);
	// tkv_value_meta *metas = (tkv_value_meta *)(object + sizeof(u16) + sizeof(u32) + sizeof(tkv_key) * keys_total);

	// (metas + value.tkv_meta_index)->tkv_value_state = TKV_STATE_CHANGED;

	// That would mean that id have to check every tkv tree for all the changed values and put them in a list after the
	// fact. Tahts why it would be cool to have an arena for changes instead.

	// TODO: have an arena for tkv value updates instead of a changed state of the value...
}

/*==============================================================================
  TYPE STRING MAPPING
  
  Maps string type names from text format to TKV_VALUE_TYPE enum values.
  This is called during parsing to determine the data type.
  
  EXTENSION POINT: When adding a new type (e.g., i32):
  1. Add new enum value to TKV_VALUE_TYPE (or use TKV_VALUE_UNUSED1)
  2. Add mapping here: if (strcmp(type, "i32") == 0) return TKV_VALUE_I32;
  3. Update parsing switch in tkv_parse_object()
  4. Update serialization in tkv_serialize_recursive()
  5. Add conversion functions (tkv_value_to_i32, tkv_value_set_i32)
===============================================================================*/
i8 tkv_string_to_tkv_type(char *type)
{
	if (strcmp(type, "bool") == 0)
		return TKV_VALUE_BOOL;
	if (strcmp(type, "i64") == 0)
		return TKV_VALUE_I64;
	if (strcmp(type, "f64") == 0)
		return TKV_VALUE_F64;
	if (strcmp(type, "str") == 0)
		return TKV_VALUE_STR;
	if (strcmp(type, "arr") == 0)
		return TKV_VALUE_ARR;
	if (strcmp(type, "tkv") == 0)
		return TKV_VALUE_TKV;
	return -1;  // Unknown type
}

/*==============================================================================
  TEMPORARY LINKED LIST NODE
  
  Used during parsing to accumulate key-value pairs before writing the final
  binary format. We build a linked list during parsing, then write all keys
  and metadata in order to the final binary structure.
===============================================================================*/
typedef struct temp_tkv_ll_node tkv_ll_node;

typedef struct temp_tkv_ll_node
{
	tkv_ll_node *next;
	tkv_key key;
} tkv_ll_node;

/*==============================================================================
  POWER-OF-10 LOOKUP TABLE
  
  Used for converting integer values to floating-point numbers during parsing.
  Allows handling of floating-point notation like "1e10" without full parsing.
  Index represents the exponent (10^0, 10^1, ..., 10^18).
===============================================================================*/
u64 q10pow[] = {
	10UL,
	100UL,
	1000UL,
	10000UL,
	100000UL,
	1000000UL,
	10000000UL,
	100000000UL,
	1000000000UL,
	10000000000UL,
	100000000000UL,
	1000000000000UL,
	10000000000000UL,
	100000000000000UL,
	1000000000000000UL,
	10000000000000000UL,
	100000000000000000UL,
	1000000000000000000UL,
	10000000000000000000UL,
};

/*==============================================================================
  PARSE TKV OBJECT FROM TEXT
  
  Parses a TKV object from text format into binary memory representation.
  
  PARSING PROCESS:
  1. Verify opening '{'
  2. For each key-value pair:
     a. Read type name (bool, i64, f64, str, arr, tkv)
     b. Read variable name and validate (1-10 chars, alphanumeric + underscore)
     c. Read '=' and parse value based on type
     d. Store compressed key in linked list, value in scratchpad
  3. After all pairs, serialize to binary format:
     - Header (node count, total size)
     - Keys section (compressed tkv_key structures)
     - Metadata section (type, state, offset for each value)
     - Values section (actual value data)
  
  MEMORY LAYOUT:
  - scratchpad_arena: Holds temporary type bytes and values during parsing
  - heap: Temporary linked list nodes for key ordering
  - tkv_arena: Final binary TKV object
  
  TO ADD A NEW TYPE (e.g., i32):
  1. Add case in value parsing switch (line ~470)
  2. Allocate storage on scratchpad and update values_size_bytes
  3. Add corresponding case in serialization switch (line ~700)
  4. Update tkv_string_to_tkv_type() to map the string
  5. Add conversion functions in header
===============================================================================*/

tkv_object tkv_parse_object(const char **tkv_source, arena *scratchpad_arena, arena *tkv_arena)
{
	const char *s = *tkv_source;

	i32 line = 1;

	token first_token = token_next(&s, &line);

	if (first_token.type != TOK_CURLY_BRACKET_LEFT)
	{
		printf("\'{\' not found at the beginning of the tkv tree\n");
		return NULL;
	}

	fflush(stdout);

	tkv_ll_node *root = NULL;
	tkv_ll_node *prev = NULL;

	u32 scratch_reset_point = scratchpad_arena->length;
	// this will save some space when going recursive

	u32 nodes_total = 0;
	u32 values_size_bytes = 0;

#define SCRATCH_ADD(type, thing) *(type *)arena_alloc(scratchpad_arena, sizeof(type)) = thing

	while (true)
	{
		token tok = token_next(&s, &line);

		if (tok.type == TOK_EOF || tok.type == TOK_CURLY_BRACKET_RIGHT)
			break;

		if (tok.type != TOK_LABEL)
		{
			printf("Expected a string for a variable type but got \'%s\' at line %d\n", tok.text, line);
			return NULL;
		}

		i8 tkv_type = tkv_string_to_tkv_type(tok.text);

		if (tkv_type < 0)
		{
			printf("Invalid typename \'%s\' at line %d\n", tok.text, line);
			return NULL;
		}

		// got token type, next token name

		token tok_current = token_next(&s, &line);

		if (tok_current.type != TOK_LABEL)
		{
			printf("Expected a string for a variable name but got \'%s\' at line %d\n", tok_current.text, line);
			return NULL;
		}

		if (strlen(tok_current.text) > TKV_KEY_LEN_MAX)
		{
			printf("Variable name \'%s\' is longer than %d bytes at line %d\n", tok_current.text, TKV_KEY_LEN_MAX,
				   line);
			return NULL;
		}

		if (!tkv_is_valid_key(tok_current.text))
		{
			printf("Variable name \'%s\'  contains invalid characters at line %d\n", tok_current.text, line);
			return NULL;
		}

		tkv_key key = tkv_make_key(tok_current.text);

		// valid tkv key is here, tok_current can be reused

		tok_current = token_next(&s, &line);

		if (tok_current.type != TOK_EQUAL)
		{
			printf("Expected an equal sign at line %d, got \'%s\'\n", line, tok_current.text);
			return NULL;
		}

		// contents of the variable will follow, we branch here based on the type
		// and build the tkv linked list for keys and values, to construct the final structure later

		// Save position before reading value token - for recursive TKV parsing
		const char *value_src = s;

		tok_current = token_next(&s, &line);

		// Allocate linked list node from heap, not scratchpad, so scratchpad is clean for types/values
		tkv_ll_node *new_node = calloc(1, sizeof(tkv_ll_node));
		if (!new_node)
			return NULL;

		new_node->key = key;

		if (prev)
			prev->next = new_node;

		if (!root)
			root = new_node;

		SCRATCH_ADD(u8, tkv_type);

		switch (tkv_type)
		{
		case TKV_VALUE_BOOL:;
			bool value_b = 0;

			if (strcmp(tok_current.text, "true") == 0)
				value_b = true;
			else if (strcmp(tok_current.text, "false") == 0)
				value_b = false;
			else
			{
				printf("Expected a boolean value at %d, got \'%s\'\n", line, tok_current.text);
				return NULL;
			}

			SCRATCH_ADD(bool, value_b);
			values_size_bytes += 1;

			break;
		case TKV_VALUE_I64:;
			i64 value_8b = 1;

			if (tok_current.type == TOK_MINUS) // hmmm
			{
				value_8b = -1;
				tok_current = token_next(&s, &line);
			}

			if (tok_current.type != TOK_NUMBER)
			{
				printf("Expected an integer value at %d, got \'%s\'\n", line, tok_current.text);
				return NULL;
			}

			value_8b *= tok_current.value;

			SCRATCH_ADD(i64, value_8b);
			values_size_bytes += sizeof(i64);

			break;
		case TKV_VALUE_F64:;
			f64 val_f64 = 1.0f;

			if (tok_current.type == TOK_MINUS)
			{
				val_f64 = -1.0f;
				tok_current = token_next(&s, &line);
			}

			if (tok_current.type != TOK_FLOAT && tok_current.type != TOK_NUMBER)
			{
				printf("Expected a float value at %d, got \'%s\'\n", line, tok_current.text);
				return NULL;
			}

			if (tok_current.type == TOK_FLOAT)
			{
				val_f64 = strtod(tok_current.text, NULL);
			}
			else if (tok_current.type == TOK_NUMBER)
			{
				// For integers serialized from floats (e.g., 1e10 becomes 10000000000)
				val_f64 = (f64)tok_current.value;
			}

			SCRATCH_ADD(f64, val_f64);
			values_size_bytes += sizeof(f64);

			break;

		case TKV_VALUE_STR:
			if (tok_current.type != TOK_STRING)
			{
				printf("Expected a string value at %d, got \'%s\'\n", line, tok_current.text);
				return NULL;
			}

			char *dest = arena_alloc(scratchpad_arena, tok_current.text_length + 1);

			strcpy(dest, tok_current.text);
			dest[tok_current.text_length] = '\0';

			values_size_bytes += tok_current.text_length + 1;

			break;
		case TKV_VALUE_ARR:
			if (tok_current.type != TOK_SQUARE_BRACKET_LEFT)
			{
				printf("Expected an \'[\' at %d, got \'%s\'\n", line, tok_current.text);
				return NULL;
			}

			// Store array metadata on scratchpad: element_size (always 1 for u8) and array_length
			u16 array_length = 0;
			u32 array_length_offset = scratchpad_arena->length;

			// Reserve space for element_size and array_length metadata
			SCRATCH_ADD(u16, 1); // element_size = 1 (all elements are u8)
			SCRATCH_ADD(u16, 0); // array_length placeholder, we'll fill this in after parsing

			tok_current = token_next(&s, &line);
			while (tok_current.type != TOK_SQUARE_BRACKET_RIGHT)
			{
				if (tok_current.type != TOK_NUMBER && tok_current.type != TOK_CHAR_LITERAL)
				{
					printf("Expected a number or a character literal value at %d, got \'%s\'\n", line,
						   tok_current.text);
					return NULL;
				}

				if (tok_current.type == TOK_NUMBER)
				{
					if (tok_current.value > 0xff || tok.value < 0)
					{
						printf("Number is out of the byte's range at %d, got \'%lld\' as an input\n", line,
							   tok_current.value);
						return NULL;
					}
					SCRATCH_ADD(u8, tok_current.value);
				}
				else
				{
					SCRATCH_ADD(u8, tok_current.text[0]);
				}

				array_length++;
				values_size_bytes += 1;

				tok_current = token_next(&s, &line);
			}

			// Write the actual array_length to the reserved location
			*(u16 *)((u8 *)scratchpad_arena->base + array_length_offset + sizeof(u16)) = array_length;

			values_size_bytes += 2 * sizeof(u16); // Account for metadata size

			break;

		case TKV_VALUE_TKV:;
			// TODO: sigsegvs here when the tkv is malformed, need to handle that better
			// Start parsing from the saved position (before the "{" token)
			// and update s to after the parsed TKV
			const char *s_child = value_src;
			tkv_object child_tkv = tkv_parse_object(&s_child, scratchpad_arena, tkv_arena);
			s = s_child; // Update parent's s to after the child TKV

			// Store a pointer to the child tkv object on the scratchpad
			// The child object remains in tkv_arena, we just store a reference to it
			SCRATCH_ADD(tkv_object, child_tkv);

			values_size_bytes += sizeof(tkv_object);

			break;

		default:
			assert(0 && "Not implemented");
		} // done writing the key-value pair

		prev = new_node;

		tok_current = token_next(&s, &line);

		if (tok_current.type != TOK_SEMICOLON)
		{
			printf("Expected an semicolon at line %d, got \'%s\'\n", line, tok_current.text);
			// printf("Token type was %s\n", token_str(tok_current.type));
			// print some more info about the current parsed line
			printf("Current line content around error: ...%.*s...\n", 50, s);
			return NULL;
		}

		nodes_total++;
	} // the tkv struct has ended. Now start writing the final tkv struct

#undef SCRATCH_ADD

	const u32 key_meta_size = nodes_total * (sizeof(tkv_key) + sizeof(tkv_value_meta));
	const u32 total_size = values_size_bytes + key_meta_size + sizeof(u32) + sizeof(u16);

	// Allocate the entire tkv object at once
	u8 *tkv_start = arena_alloc(tkv_arena, total_size);

	// Calculate section pointers
	u8 *header = tkv_start;
	u8 *keys_section = header + sizeof(u16) + sizeof(u32);
	u8 *metas_section = keys_section + nodes_total * sizeof(tkv_key);
	u8 *values_section = metas_section + nodes_total * sizeof(tkv_value_meta);

	// Write header
	*(u16 *)header = nodes_total;
	*(u32 *)(header + sizeof(u16)) = total_size;

	// Write keys from linked list
	tkv_ll_node *node = root;
	u16 key_index = 0;

	while (node != NULL && key_index < nodes_total)
	{
		*(tkv_key *)(keys_section + key_index * sizeof(tkv_key)) = node->key;
		node = node->next;
		key_index++;
	}

	// Walk scratchpad to extract types and values, write metadata and values
	u8 *scratch_ptr = (u8 *)scratchpad_arena->base + scratch_reset_point;
	u8 *scratch_end = (u8 *)scratchpad_arena->base + scratchpad_arena->length;
	u8 *values_write_ptr = values_section;
	u32 value_index = 0;

	while (value_index < nodes_total && scratch_ptr < scratch_end)
	{
		u8 type = *scratch_ptr;
		scratch_ptr += sizeof(u8);

		u32 value_size = 0;
		u32 value_offset = (u32)(values_write_ptr - tkv_start);

		switch (type)
		{
		case TKV_VALUE_BOOL:
			value_size = sizeof(bool);
			memcpy(values_write_ptr, scratch_ptr, value_size);
			scratch_ptr += value_size;
			values_write_ptr += value_size;
			break;

		case TKV_VALUE_I64:
			value_size = sizeof(i64);
			memcpy(values_write_ptr, scratch_ptr, value_size);
			scratch_ptr += value_size;
			values_write_ptr += value_size;
			break;

		case TKV_VALUE_F64:
			value_size = sizeof(f64);
			memcpy(values_write_ptr, scratch_ptr, value_size);
			scratch_ptr += value_size;
			values_write_ptr += value_size;
			break;

		case TKV_VALUE_STR:;
			u32 str_len = strlen((char *)scratch_ptr) + 1;
			value_size = str_len;
			memcpy(values_write_ptr, scratch_ptr, str_len);
			scratch_ptr += str_len;
			values_write_ptr += str_len;
			break;

		case TKV_VALUE_ARR:;
			u16 element_size = *(u16 *)scratch_ptr;
			u16 array_length = *(u16 *)(scratch_ptr + sizeof(u16));
			value_size = 2 * sizeof(u16) + element_size * array_length;
			memcpy(values_write_ptr, scratch_ptr, value_size);
			scratch_ptr += value_size;
			values_write_ptr += value_size;
			break;

		case TKV_VALUE_TKV:;
			// Read the pointer to the child TKV object that was stored on the scratchpad
			tkv_object child_ptr = *(tkv_object *)scratch_ptr;
			value_size = sizeof(tkv_object);
			memcpy(values_write_ptr, &child_ptr, value_size);
			scratch_ptr += value_size;
			values_write_ptr += value_size;
			break;

		default:
			assert(0 && "Unknown tkv type");
			break;
		}

		// Write metadata
		tkv_value_meta meta = {
			.tkv_value_type = type,
			.tkv_value_state = TKV_STATE_CONST,
			.tkv_value_offset = value_offset,
		};

		*(tkv_value_meta *)(metas_section + value_index * sizeof(tkv_value_meta)) = meta;

		value_index++;
	}

	// reset consumed space for the caller above us
	// without it, each child tkv would eat double-ish the memory on the scratchpad, while leaving the result arena
	// completely empty.
	scratchpad_arena->length = scratch_reset_point;

	// Free the temporary linked list nodes
	tkv_ll_node *next_node;
	node = root;
	while (node != NULL)
	{
		next_node = node->next;
		SAFE_FREE(node);
		node = next_node;
	}

	// Update the caller's source pointer to after the parsed TKV
	*tkv_source = s;

	return (tkv_object)tkv_start;
}

/*==============================================================================
  TYPE NAME LOOKUP
  
  Returns the string name for a TKV_VALUE_TYPE enum value.
  Used during serialization to write type names in text output.
  
  EXTENSION POINT: When adding a new type, add a corresponding case here.
===============================================================================*/
static const char *tkv_type_name(u8 type)
{
	switch (type)
	{
	case TKV_VALUE_BOOL:
		return "bool";
	case TKV_VALUE_I64:
		return "i64";
	case TKV_VALUE_F64:
		return "f64";
	case TKV_VALUE_STR:
		return "str";
	case TKV_VALUE_ARR:
		return "arr";
	case TKV_VALUE_TKV:
		return "tkv";
	default:
		return "unknown";
	}
}

/*==============================================================================
  CALCULATE SERIALIZED VALUE SIZE
  
  Estimates the byte count needed to serialize a single value to text format.
  This is used for pre-allocating the output buffer.
  
  Note: This is an estimate that may vary based on the actual value content.
  For example, floating-point values may have different precision when serialized.
===============================================================================*/
static u32 tkv_calculate_value_size(tkv_value value)
{
	u8 type = value.meta.tkv_value_type;
	char temp[64];

	switch (type)
	{
	case TKV_VALUE_BOOL:
		// "true" or "false"
		return 5;

	case TKV_VALUE_I64:;
		i64 i = *(i64 *)value.ptr;
		// Max i64: -9223372036854775808 = 20 chars
		return snprintf(temp, sizeof(temp), "%lld", i);

	case TKV_VALUE_F64:;
		f64 f = *(f64 *)value.ptr;
		// With %.17g format, max ~25 chars
		return snprintf(temp, sizeof(temp), "%.17g", f);

	case TKV_VALUE_STR:;
		char *str = (char *)value.ptr;
		// String with quotes: 2 + length
		return 2 + strlen(str);

	case TKV_VALUE_ARR:;
		tkv_array arr = tkv_value_to_arr(value);
		// Array format: "[ " + bytes + "]"
		// Each byte: "0xXX " (~5 chars), conservative estimate
		return 3 + (arr.array_length * 5);

	case TKV_VALUE_TKV:
		// Placeholder for nested TKV (will be recursively calculated)
		return 5;  // "{...}"

	default:
		return 3;  // "???"
	}
}

// Forward declaration for recursive size calculation
static u32 tkv_calculate_size_recursive(tkv_object object, u32 indent_level);

// Calculate the size needed to serialize a complete TKV object
static u32 tkv_calculate_size_recursive(tkv_object object, u32 indent_level)
{
	u32 total_size = 0;
	u32 indent_spaces = indent_level * 4;

	// Open brace + newline
	total_size += 2;

	u16 num_keys = tkv_object_get_length(object);

	for (u16 i = 0; i < num_keys; i++)
	{
		// Indentation
		total_size += indent_spaces;

		tkv_key key = tkv_object_get_key(object, i);
		char key_name[TKV_KEY_LEN_MAX + 1];
		tkv_key_to_str(key, key_name);
		key_name[key.size] = '\0';

		tkv_value val = tkv_get_value(object, key_name);
		const char *type_str = tkv_type_name(val.meta.tkv_value_type);

		// "type key = "
		total_size += strlen(type_str) + 1 + strlen(key_name) + 3; // +3 for " = "

		// Handle nested tkv specially
		if (val.meta.tkv_value_type == TKV_VALUE_TKV)
		{
			tkv_object child = tkv_value_to_tkv(val);
			total_size += tkv_calculate_size_recursive(child, indent_level + 1);
		}
		else
		{
			total_size += tkv_calculate_value_size(val);
		}

		// ";\n"
		total_size += 2;
	}

	// Indentation for closing brace
	total_size += indent_spaces;

	// Closing brace
	total_size += 1;

	return total_size;
}

/*==============================================================================
  SERIALIZE SINGLE VALUE TO TEXT
  
  Converts a TKV value to its text representation in a buffer.
  Returns the number of bytes written (not including null terminator).
  
  NOTE: Buffer may overflow. Caller is responsible for pre-allocation.
  
  EXTENSION POINT: When adding a new type (e.g., i32):
  1. Add a case here to format the value appropriately
  2. Use snprintf with appropriate format string
  3. Update tkv_calculate_value_size() to estimate size for your type
===============================================================================*/
u32 tkv_serialize_value(u8 *buffer, u32 buffer_size, tkv_value value)
{
	u32 written = 0;
	u8 type = value.meta.tkv_value_type;

	switch (type)
	{
	case TKV_VALUE_BOOL:;
		bool b = *(bool *)value.ptr;
		written = snprintf((char *)buffer, buffer_size, "%s", b ? "true" : "false");
		break;

	case TKV_VALUE_I64:;
		i64 i = *(i64 *)value.ptr;
		written = snprintf((char *)buffer, buffer_size, "%lld", i);
		break;

	case TKV_VALUE_F64:;
		f64 f = *(f64 *)value.ptr;
		written = snprintf((char *)buffer, buffer_size, "%.17g", f);
		break;

	case TKV_VALUE_STR:;
		char *str = (char *)value.ptr;
		written = snprintf((char *)buffer, buffer_size, "\"%s\"", str);
		break;

	case TKV_VALUE_ARR:;
		tkv_array arr = tkv_value_to_arr(value);
		u32 bytes_left = (buffer_size > written) ? (buffer_size - written) : 0;
		written += snprintf((char *)(buffer + written), bytes_left, "[ ");

		for (u16 i = 0; i < arr.array_length; i++)
		{
			u8 byte_val = arr.bytes[i];
			bytes_left = (buffer_size > written) ? (buffer_size - written) : 0;
			u32 this_write = snprintf((char *)(buffer + written), bytes_left, "0x%02x ", byte_val);
			written += this_write;
		}

		// Always write closing bracket, even if buffer overflowed
		bytes_left = (buffer_size > written) ? (buffer_size - written) : 0;
		u32 closing_write = snprintf((char *)(buffer + written), bytes_left, "]");
		written += closing_write;
		break;

	case TKV_VALUE_TKV:;
		// For nested objects, we'll handle this differently
		// Just write a placeholder here
		written = snprintf((char *)buffer, buffer_size, "{...}");
		break;

	default:
		written = snprintf((char *)buffer, buffer_size, "???");
		break;
	}

	return written;
}

// Recursive helper to serialize with proper indentation
static u32 tkv_serialize_recursive(tkv_object object, u8 *buffer, u32 buffer_size, u32 indent_level)
{
	u32 written = 0;
	u32 indent_spaces = indent_level * 4;

	// Open brace
	if (written < buffer_size)
		written += snprintf((char *)(buffer + written), buffer_size - written, "{\n");

	u16 num_keys = tkv_object_get_length(object);

	for (u16 i = 0; i < num_keys; i++)
	{
		// Indentation
		if (written < buffer_size)
		{
			for (u32 j = 0; j < indent_spaces && written < buffer_size; j++)
				buffer[written++] = ' ';
		}

		tkv_key key = tkv_object_get_key(object, i);
		char key_name[TKV_KEY_LEN_MAX + 1];
		tkv_key_to_str(key, key_name);
		key_name[key.size] = '\0';

		tkv_value val = tkv_get_value(object, key_name);
		const char *type_str = tkv_type_name(val.meta.tkv_value_type);

		// Write "type name = "
		if (written < buffer_size)
			written += snprintf((char *)(buffer + written), buffer_size - written, "%s %s = ", type_str, key_name);

		// Handle nested tkv specially
		if (val.meta.tkv_value_type == TKV_VALUE_TKV)
		{
			tkv_object child = tkv_value_to_tkv(val);
			u32 child_written =
				tkv_serialize_recursive(child, buffer + written, buffer_size - written, indent_level + 1);
			written += child_written;
		}
		else
		{
			// Serialize the value
			u8 temp_buffer[8 * 1024];
			u32 value_size = tkv_serialize_value(temp_buffer, sizeof(temp_buffer), val);

			if (written + value_size < buffer_size)
			{
				memcpy(buffer + written, temp_buffer, value_size);
			}
			// Always increment written to track total size needed
			written += value_size;
		}

		snprintf((char *)(buffer + written), buffer_size - written, ";");
		written += 1;

		// Newline
		if (written < buffer_size)
			written += snprintf((char *)(buffer + written), buffer_size - written, "\n");
	}

	// Indentation for closing brace
	if (written < buffer_size)
	{
		for (u32 j = 0; j < indent_spaces && written < buffer_size; j++)
			buffer[written++] = ' ';
	}

	// Close brace
	if (written < buffer_size)
		written += snprintf((char *)(buffer + written), buffer_size - written, "}");

	return written;
}

char *tkv_serialize_object(tkv_object object, arena *output_arena)
{
	if (!object)
		return NULL;

	// Calculate exact size needed for serialization
	u32 needed_size = tkv_calculate_size_recursive(object, 0);

	// Add padding for array/string size variability
	// Use 5x the calculated size + 32KB base as a good balance
	u32 alloc_size = needed_size * 5 + 32768;

	// Cap to prevent excessive allocations that exceed Windows command-line limits
	if (alloc_size > 32000) // Stay under ~32KB command-line limit
		alloc_size = 32000;

	u8 *buffer = arena_alloc(output_arena, alloc_size);
	if (!buffer)
		return NULL;

	u32 written = tkv_serialize_recursive(object, buffer, alloc_size, 0);

	// Null terminate
	if (written < alloc_size)
		buffer[written] = '\0';
	else
		buffer[alloc_size - 1] = '\0';

	return (char *)buffer;
}

u8 *tkv_serialize_for_network(tkv_object object, arena *output_arena, u32 *out_len)
{
	if (!object)
		return NULL;

	char *s = tkv_serialize_object(object, output_arena);
	if (!s)
		return NULL;

	if (out_len)
		*out_len = (u32)strlen(s);

	return (u8 *)s;
}