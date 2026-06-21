#ifndef TKV_H
#define TKV_H

#include "general.h"
#include "arena.h"
#include "vec_math.h"

#include <assert.h>

/*==============================================================================
  TKV (Typed Key-Value) Tree Format
  ==============================================================================
  
  TKV is a hierarchical, serializable data structure for game object
  configuration and networking. It combines human-readable text format with
  efficient binary storage.
  
  TEXT FORMAT EXAMPLE:
  {
    bool thing_1 = true;
    i64 Funny = 60065;
    f64 Pi_4 = 0.78539816339744830962;
    str T_Str = "test string value";
    arr array_1 = [ 0xf3 0x81 0xa8 0xcc 0x10 0x01 0x0c ];
    tkv Subtree = {
      i64 somethin = 1300;
    };
  }
  
  KEY CHARACTERISTICS:
  - Variable names: 1-10 characters, alphanumeric + underscore only
  - Values: Strongly typed with no implicit conversion
  - Nesting: tkv values can contain nested TKV structures
  - States: Values can be marked as CONST, VOLATILE, NETWORKABLE, or CHANGED
  
  BINARY FORMAT OVERVIEW:
  [Header (6 bytes)] [Keys section] [Metadata section] [Values section]
  
  Header:
    u16 node_count      - Number of key-value pairs
    u32 object_size     - Total size of this TKV object in bytes
  
  Keys section:
    tkv_key[node_count] - Compressed variable names (8 bytes each)
    
  Metadata section:
    tkv_value_meta[node_count] - Type, state, and value offset (4 bytes each)
    
  Values section:
    [variable-length value data] - Actual value contents, referenced by offset
  
  EXTENSION GUIDE - Adding New Types:
  ====================================
  To support new data types (e.g., i8, u16, i32, etc.):
  
  1. Add enum entry in TKV_VALUE_TYPE:
     TKV_VALUE_I32,  (or replace an UNUSED slot)
  
  2. Add conversion function in tkv.c:
     i32 tkv_value_to_i32(tkv_value value);
     void tkv_value_set_i32(tkv_value value, i32 new_val);
  
  3. Update tkv_string_to_tkv_type() to map "i32" string
  
  4. Update parsing switch in tkv_parse_object() with new case
  
  5. Update serialization switch in tkv_serialize_recursive()
  
  6. Update type name function tkv_type_name()
  
  Current available slots: TKV_VALUE_UNUSED1, TKV_VALUE_UNUSED2
  
===============================================================================*/

typedef enum
{
	TKV_VALUE_BOOL,   // bool - 1 byte, true/false

  TKV_VALUE_I8,    // i8 - 1 byte, signed 8-bit integer
  TKV_VALUE_I16,   // i16 - 2 bytes, signed 16-bit integer
  TKV_VALUE_I32,   // i32 - 4 bytes, signed 32-bit integer
	TKV_VALUE_I64,    // i64 - 8 bytes, signed 64-bit integer

  TKV_VALUE_U8,    // u8 - 1 byte, unsigned 8-bit integer
  TKV_VALUE_U16,   // u16 - 2 bytes, unsigned 16
  TKV_VALUE_U32,   // u32 - 4 bytes, unsigned 32-bit integer
  TKV_VALUE_U64,   // u64 - 8 bytes, unsigned 64-bit integer

  TKV_VALUE_F32,   // f32 - 4 bytes, IEEE 754 single precision float
	TKV_VALUE_F64,    // f64 - 8 bytes, IEEE 754 double precision float

	TKV_VALUE_STR,    // str - null-terminated string, variable length
	TKV_VALUE_ARR,    // arr - typed byte array with element_size metadata
  TKV_VALUE_VEC3,   // vec3 - 12 bytes, three f32 values for 3D vector
  TKV_VALUE_QUAT,   // quat - 16 bytes, four f32 values for quaternion

	TKV_VALUE_TKV,    // tkv - nested TKV object (pointer)

	TKV_VALUE_LAST,
} TKV_VALUE_TYPE;

static_assert(TKV_VALUE_LAST == 16, "");

typedef enum
{
	TKV_STATE_CONST,         // Value cannot be modified
	TKV_STATE_VOLATILE,      // Value can be modified locally
	TKV_STATE_NETWORKABLE,   // Value can be synced over network
	TKV_STATE_CHANGED,       // Value was modified (for network sync tracking)
	TKV_STATE_LAST
} TKV_VALUE_STATE;

static_assert(TKV_STATE_LAST == 4, "");

#define TKV_KEY_LEN_MAX 10  // Maximum variable name length

/*==============================================================================
  KEY COMPRESSION
  
  Variable names (keys) are compressed using 6-bit encoding to save space:
  - Each character is mapped to a 6-bit value (range 0-63)
  - Supports 26 lowercase, 26 uppercase, 10 digits, underscore, and null terminator
  - Up to 10 characters fit in a single 64-bit integer with 4-bit length prefix
  
  Compression mapping:
    0      -> '\0' (null terminator)
    1-26   -> 'a'-'z'
    27-52  -> 'A'-'Z'
    53-62  -> '0'-'9'
    63     -> '_'
  
  Layout: [4-bit size][60-bit payload] = 64 bits total
===============================================================================*/

typedef struct
{
	union
	{
		struct
		{
			u64 size : 4;      // Number of characters in the key (1-10)
			u64 payload : 60;  // 6-bit encoded characters (10 chars * 6 bits)
		};
		u64 whole;  // View as single 64-bit integer for comparison
	};
} tkv_key;

static_assert(sizeof(tkv_key) == 8, "");

#define TKV_INVALID_KEY                                                                                                \
	(tkv_key)                                                                                                          \
	{                                                                                                                  \
	}

i8 util_compress_char(char val);
char util_decompress_char(i8 c_val);

tkv_key tkv_make_key(const char *input);
void tkv_key_to_str(const tkv_key key, char *out);

typedef struct
{
	union
	{
		struct
		{
			// Bit-field layout (32 bits total):
			u32 tkv_value_type : 4;    // TKV_VALUE_TYPE: type discriminant for this value
			u32 tkv_value_state : 2;   // TKV_VALUE_STATE: CONST, VOLATILE, NETWORKABLE, CHANGED
			u32 tkv_value_offset : 26;  // Byte offset to value in this TKV object (0-128MB)
		};
		u32 whole;  // View as single 32-bit integer for compact storage
	};
} tkv_value_meta;

static_assert(sizeof(tkv_value_meta) == 4, "");

#define TKV_INVALID_META                                                                                               \
	(tkv_value_meta)                                                                                                   \
	{                                                                                                                  \
		.whole = UINT_MAX                                                                                              \
	}

typedef struct
{
	void *ptr;
	tkv_value_meta meta;
	u16 tkv_meta_index;
	u16 unused;
} tkv_value;

/*
INTERNAL BINARY FORMAT LAYOUT:
==============================

When a TKV object is serialized to memory, it uses this layout:

[0x00] u16 node_count
[0x02] u32 total_size_bytes

[0x06] tkv_key keys[node_count]               (8 bytes each)
       ^--Compressed variable names

[0x06 + N*8] tkv_value_meta metas[node_count] (4 bytes each)
             ^--Type, state, and offset for each value

[0x06 + N*8 + N*4] u8 values[...]             (variable-length)
                   ^--Actual value data, offset referenced by metadata

This design allows:
- O(1) random access to any key-value pair
- Compact storage with 6-bit compressed keys
- Type safety through metadata encoding
- Efficient network serialization
*/

typedef void *tkv_object;

typedef struct
{
	u16 element_size;
	u16 array_length;
	u8 *bytes;
} tkv_array;

tkv_value tkv_get_value(tkv_object object, const char *key_str);
tkv_value tkv_traverse_get_value(tkv_object object, const char *path);
tkv_object tkv_value_get_root(tkv_value value);

#define DEF_TKV_TO_TYPE(type, enum_type) type tkv_value_to_##type(tkv_value value);

DEF_TKV_TO_TYPE(bool, BOOL)
DEF_TKV_TO_TYPE(i8, I8)
DEF_TKV_TO_TYPE(i16, I16)
DEF_TKV_TO_TYPE(i32, I32)
DEF_TKV_TO_TYPE(i64, I64)
DEF_TKV_TO_TYPE(u8, U8)
DEF_TKV_TO_TYPE(u16, U16)
DEF_TKV_TO_TYPE(u32, U32)
DEF_TKV_TO_TYPE(u64, U64)
DEF_TKV_TO_TYPE(f32, F32)
DEF_TKV_TO_TYPE(f64, F64)

char *tkv_value_to_str(tkv_value value);
tkv_array tkv_value_to_arr(tkv_value value);
vec3 tkv_value_to_vec3(tkv_value value);
quaternion tkv_value_to_quat(tkv_value value);
tkv_object tkv_value_to_tkv(tkv_value value);

#define DEF_TKV_SETTER(type, enum_type) void tkv_value_set_##type(tkv_value value, type new_val);

DEF_TKV_SETTER(bool, BOOL)
DEF_TKV_SETTER(i8, I8)
DEF_TKV_SETTER(i16, I16)
DEF_TKV_SETTER(i32, I32)
DEF_TKV_SETTER(i64, I64)
DEF_TKV_SETTER(u8, U8)
DEF_TKV_SETTER(u16, U16)
DEF_TKV_SETTER(u32, U32)
DEF_TKV_SETTER(u64, U64)
DEF_TKV_SETTER(f32, F32)
DEF_TKV_SETTER(f64, F64)

void tkv_value_set_vec3(tkv_value value, vec3 new_val);
void tkv_value_set_quat(tkv_value value, quaternion new_val);

void tkv_value_set_changed(tkv_value value);

tkv_object tkv_parse_object(const char **tkv_source, arena *scratchpad_arena, arena *tkv_arena);
u32 tkv_serialize_value(u8 *buffer, u32 buffer_size, tkv_value value);
char *tkv_serialize_object(tkv_object object, arena *output_arena);

// Convenience helper: serialize an object for sending over the network.
// Returns a buffer allocated from `output_arena` and writes its length to `out_len` if non-NULL.
// The returned buffer is a nul-terminated text representation (same as `tkv_serialize_object`).
u8 *tkv_serialize_for_network(tkv_object object, arena *output_arena, u32 *out_len);

/*==============================================================================
  PORTABLE BINARY VALUE SERIALIZERS

  These functions write a typed value into a buffer in TKV binary format (the
  same format used in the values section of a serialized TKV object). They
  return the number of bytes written.

  Use these standalone when you need to serialize individual values without
  the full TKV object overhead - e.g., for network sync, file I/O, or
  preparing data for tkv_object_add_field().
===============================================================================*/

u32 tkv_write_value_bool(u8 *buffer, bool value);
u32 tkv_write_value_i8(u8 *buffer, i8 value);
u32 tkv_write_value_i16(u8 *buffer, i16 value);
u32 tkv_write_value_i32(u8 *buffer, i32 value);
u32 tkv_write_value_i64(u8 *buffer, i64 value);
u32 tkv_write_value_u8(u8 *buffer, u8 value);
u32 tkv_write_value_u16(u8 *buffer, u16 value);
u32 tkv_write_value_u32(u8 *buffer, u32 value);
u32 tkv_write_value_u64(u8 *buffer, u64 value);
u32 tkv_write_value_f32(u8 *buffer, f32 value);
u32 tkv_write_value_f64(u8 *buffer, f64 value);
u32 tkv_write_value_str(u8 *buffer, const char *str);
u32 tkv_write_value_arr(u8 *buffer, u16 element_size, u16 array_length, const u8 *bytes);
u32 tkv_write_value_vec3(u8 *buffer, vec3 value);
u32 tkv_write_value_quat(u8 *buffer, quaternion value);
u32 tkv_write_value_tkv(u8 *buffer, tkv_object value);

/*==============================================================================
  ADD FIELD TO EXISTING TKV OBJECT

  Creates a new TKV object with an additional field appended. The original
  object is not modified. The result is allocated from the provided arena.

  Parameters:
    object  - Existing binary TKV object to extend
    key     - Variable name (1-10 chars, alphanumeric + underscore)
    type    - TKV_VALUE_TYPE enum for the new field
    state   - TKV_VALUE_STATE for the new field (CONST, VOLATILE, etc.)
    value   - Pointer to the value data (interpreted based on type):
              Scalar types (bool, i8-u64, f32-f64, vec3, quat):
                pointer to a variable of that type
              STR: the string itself (const char*)
              ARR: pointer to a tkv_array struct
              TKV: the tkv_object (void*) itself
    arena   - Arena to allocate the resulting TKV object from

  Returns the new TKV object, or NULL if the key is invalid or already exists.
===============================================================================*/
tkv_object tkv_object_add_field(tkv_object object, const char *key, u8 type, u8 state, const void *value, arena *arena);

/*==============================================================================
  STREAM I/O — TKV ↔ file_system bridge

  These functions serialize/deserialize a TKV object tree through `stream_t`,
  handling endianness and nested TKV_VALUE_TKV children transparently.

  The stream format is a self-describing tagged binary:
    [u16 node_count]
    for each node:
      [u64 compressed_key] [u8 type] [u8 state]
      [value_data: depends on type, uses stream_write for endian-aware I/O]
      TKV_VALUE_TKV: recurses with its own [u16 node_count + children]

  Callers must #include "data_io.h" for the stream_t definition.
===============================================================================*/

// Create an empty TKV object with 0 fields (header only).
// Useful as the starting point for building metadata programmatically.
tkv_object tkv_object_create_empty(arena *arena);

// Serialize a TKV object tree to a stream (endian-aware, handles nesting).
// s is a stream_t* (from data_io.h). Declared as void* to avoid header dependency.
u8 tkv_write_to_stream(tkv_object object, void *s);

// Read a TKV object tree from a stream.
// s is a stream_t* (from data_io.h). Declared as void* to avoid header dependency.
// scratchpad_arena: temporary storage during parsing (can be reset after).
// tkv_arena: holds the resulting TKV object tree.
tkv_object tkv_read_from_stream(void *s, arena *scratchpad_arena, arena *tkv_arena);

#endif