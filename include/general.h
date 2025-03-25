#ifndef GENERAL_H
#define GENERAL_H 1

#include <assert.h>
#include <stdbool.h>
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

#define LEVELS_FOLDER "levels"
#define REGISTRIES_FOLDER "registries"
#define REGISTRY_TEXTURES_FOLDER "textures"
#define SCRIPTS_FOLDER "scripts"

#define SAFE_FREE(ptr) \
    if (ptr)           \
        free(ptr);     \
    ptr = 0;           \
    LOG_DEBUG("freeing %p", ptr);

#define CHECK_PTR(ptr)                                   \
    if (!(ptr))                                          \
    {                                                    \
        LOG_ERROR("check failed: '" #ptr "' is NULL\n"); \
        return FAIL;                                     \
    }

#define CHECK_PTR_NORET(ptr)                             \
    if (!(ptr))                                          \
    {                                                    \
        LOG_ERROR("check failed: '" #ptr "' is NULL\n"); \
        return;                                          \
    }

#define CHECK(expression)                                           \
    if (expression)                                                 \
    {                                                               \
        LOG_ERROR("check failed: '" #expression " is positive'\n"); \
        return FAIL;                                                \
    }

#define CHECK_NORET(expression)                                     \
    if (expression)                                                 \
    {                                                               \
        LOG_ERROR("check failed: '" #expression " is positive'\n"); \
        return;                                                     \
    }

#endif
