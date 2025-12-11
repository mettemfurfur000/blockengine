#ifndef GENERAL_H
#define GENERAL_H 1

// required, dont get fooled by the clangd error
#include "backtrace.h"
#include "logging.h"
#include <assert.h>
#include <stdbool.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

static_assert(sizeof(u8) == 1, "");
static_assert(sizeof(u16) == 2, "");
static_assert(sizeof(u32) == 4, "");
static_assert(sizeof(u64) == 8, "");

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

static_assert(sizeof(i8) == 1, "");
static_assert(sizeof(i16) == 2, "");
static_assert(sizeof(i32) == 4, "");
static_assert(sizeof(i64) == 8, "");

#define SUCCESS 0
#define FAIL -1

#define FPS 60
#define TPS 10

#define MAX_PATH_LENGTH 512

#define SUCCESSFUL(op) ((op) == SUCCESS)

#define SAFE_FREE(ptr)                                                                                                 \
    if (ptr)                                                                                                           \
        free(ptr);                                                                                                     \
    ptr = 0;

// Custom assert that prints backtrace on failure
#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#undef assert
#define assert(condition)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            LOG_ERROR("Assertion failed: %s", #condition);                                                             \
            LOG_ERROR("  File: %s:%d", __FILE__, __LINE__);                                                            \
            print_backtrace();                                                                                         \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)
#endif

#endif
