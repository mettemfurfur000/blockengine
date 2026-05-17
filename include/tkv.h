#ifndef TKV_H
#define TKV_H

#include "general.h"
#include "arena.h"

#include <assert.h>

/*

imagined format:

{
	bool thing_1 = true
	i64 Funny = 60065
	f64 Pi_4 = 0.78539816339744830962
	str TestStringValue = "test string value" 					<--- error - too long variable name
	str T_Str = "test string value"
	arr array_1 = [ 0xf3 0x81 0xa8 0xcc 0x10 0x01 0x0c ]
	tkv Subtree = {
		i64 somethin = 1300
	}
}

*/

typedef enum
{
	TKV_VALUE_BOOL,
	TKV_VALUE_I64,
	TKV_VALUE_F64,
	TKV_VALUE_STR,
	TKV_VALUE_ARR,
	TKV_VALUE_TKV,
	TKV_VALUE_UNUSED1,
	TKV_VALUE_UNUSED2,
	TKV_VALUE_LAST,
} TKV_VALUE_TYPE;

static_assert(TKV_VALUE_LAST % 2 == 0, "");

typedef enum
{
	TKV_STATE_CONST,
	TKV_STATE_VOLATILE,
	TKV_STATE_NETWORKABLE,
	TKV_STATE_CHANGED,
	TKV_STATE_LAST
} TKV_VALUE_STATE;

static_assert(TKV_STATE_LAST % 2 == 0, "");

#define TKV_KEY_LEN_MAX 10

typedef struct
{
	union
	{
		struct
		{
			u64 size : 4;
			u64 payload : TKV_KEY_LEN_MAX * 6;
		};
		u64 whole;
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
void tkv_unmangle_key(const tkv_key key, char *out);

typedef struct
{
	union
	{
		struct
		{

			u32 tkv_value_offset : 27;
			u32 tkv_value_state : 2;
			u32 tkv_value_type : 3;
		};
		u32 whole;
	};
} tkv_value_meta;

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
internal format:

N length : u16
J obj_size_bytes : u32

u64 key_list[N] = {
	key_entry
}

u32 value_meta[N] = {
	type : 3 	}
	state : 2	} 
	offset : 27	}
} 

u8 values[?] = {} 

*/

typedef void *tkv_object;

typedef struct
{
	u16 element_size;
	u16 array_length;
	u8 *bytes;
} tkv_array;

tkv_value tkv_get_value(tkv_object object, const char *key_str);
tkv_object tkv_value_get_root(tkv_value value);

bool tkv_value_to_bool(tkv_value value);
i64 tkv_value_to_i64(tkv_value value);
f64 tkv_value_to_f64(tkv_value value);
char *tkv_value_to_str(tkv_value value);
tkv_array tkv_value_to_arr(tkv_value value);
tkv_object tkv_value_to_tkv(tkv_value value);

void tkv_value_set_bool(tkv_value value, bool new_val);
void tkv_value_set_i64(tkv_value value, i64 new_val);
void tkv_value_set_f64(tkv_value value, f64 new_val);

tkv_object tkv_parse_object(const char **tkv_source, arena *scratchpad_arena, arena *tkv_arena);
char *tkv_serialize_object(tkv_object object, arena *output_arena);

#endif