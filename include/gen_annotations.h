#ifndef GEN_ANNOTATIONS_H
#define GEN_ANNOTATIONS_H 1

#include "general.h"

#define MAX_LUA_NAME 64
#define MAX_SIG_LEN 256

typedef enum
{
    ANNOT_CLASS,
    ANNOT_LIB
} annotation_type;

typedef struct
{
    char name[MAX_LUA_NAME];
    char params[MAX_SIG_LEN];
    char returns[MAX_SIG_LEN];
    bool has_sig;
} func_entry;

typedef struct
{
    annotation_type type;
    char lua_name[MAX_LUA_NAME];
    func_entry *funcs;
    u32 func_count;
    u32 func_cap;
} annotation;

int gen_annotations(const char **src_files, u32 file_count, const char *output_path);

#endif
