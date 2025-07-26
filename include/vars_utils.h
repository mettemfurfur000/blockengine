#ifndef VARS_UTILS_H
#define VARS_UTILS_H 1

#include "general.h"
#include "hashtable.h"

typedef enum
{
    T_UNKNOWN,
    T_U8,
    T_U16,
    T_U32,
    T_U64,
    T_I8,
    T_I16,
    T_I32,
    T_I64,
    T_STR
} VAR_TYPE;

u8 vars_parse(const char *str_to_cpy, blob *b);

#endif