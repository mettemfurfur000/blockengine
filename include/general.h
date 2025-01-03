#ifndef GENERAL_H
#define GENERAL_H 1

#include <assert.h>
#include "logging.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

#define SUCCESS 0
#define FAIL -1

#define SUCCESSFUL(op) ((op) == SUCCESS)

#define SAFE_FREE(ptr) \
    if (ptr)           \
        free(ptr);     \
    ptr = 0;

#define PTR_CHECKER(ptr)             \
    if (!ptr)                        \
    {                                \
        LOG_ERROR(#ptr "is NULL\n"); \
        return FAIL;                 \
    }

#endif
