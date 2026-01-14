#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H 1

#include "endianless.h"
#include "general.h"
#include "level.h"

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define COMPRESS_LEVEL 1
#define BUFFER_SIZE 8192

// #define FILE_SYSTEM_DEBUG

/* compression-related stuff */

typedef enum
{
    STREAM_PLAIN, /* ordinary FILE* */
    STREAM_GZIP   /* gzFile from zlib   */
} stream_mode_t;

/* The struct that all functions will actually receive. */
typedef struct
{
    stream_mode_t mode;
    union
    {
        FILE *plain; /* when mode == STREAM_PLAIN */
        gzFile gz;   /* when mode == STREAM_GZIP  */
    } handle;
} stream_t;

/* Convenience alias: use `stream` where a pointer to `stream_t` is expected.
    Declaring variables as `stream` (not `stream_t`) makes it harder to
    accidentally take their address when calling the READ/WRITE macros. */
typedef stream_t *stream;

/* Compatibility shim – lets us keep the old signatures */
#define AS_STREAM(p) ((stream_t *)(p))

extern u8 flip_buf[8];

static inline int stream_write(const void *buf, size_t size, stream_t *s)
{
    if (size % 2 == 0 && size <= 8 && size > 0)
    {
        memcpy(flip_buf, buf, size);
        make_endianless(flip_buf, size);
        buf = flip_buf;
    }

    if (s->mode == STREAM_PLAIN)
    {
        return fwrite(buf, 1, size, s->handle.plain) == size ? 0 : -1;
    }
    else
    { /* STREAM_GZIP */
        int written = gzwrite(s->handle.gz, buf, (unsigned int)size);

        if (written <= 0)
        {
            int errnum = 0;
            const char *msg = gzerror(s->handle.gz, &errnum);
            if (errnum == Z_OK)
                return 0; // ?? no error?
            if (Z_ERRNO == errnum)
                LOG_ERROR("gzwrite failed, errno: %s - %d", strerror(errno), errno);
            else
                LOG_ERROR("gzwrite failed: %s - %d", msg, errnum);
        }

        return written == (int)size ? 0 : -1;
    }
}

static inline int stream_read(void *buf, size_t size, stream_t *s)
{
    int ret_code = true;
    if (s->mode == STREAM_PLAIN)
    {
        ret_code = fread(buf, 1, size, s->handle.plain) == size ? 0 : -1;
    }
    else
    { /* STREAM_GZIP */
        int read = gzread(s->handle.gz, buf, (unsigned int)size);
        if (read <= 0)
        {
            int errnum = 0;
            const char *msg = gzerror(s->handle.gz, &errnum);
            if (errnum == Z_OK)
                return 0; // ?? no error?
            if (Z_ERRNO == errnum)
                LOG_ERROR("gzwrite failed, errno: %s - %d", strerror(errno), errno);
            else
                LOG_ERROR("gzwrite failed: %s - %d", msg, errnum);
        }
        ret_code = read == (int)size ? 0 : -1;
    }

    if (size % 2 == 0 && size < 9 && size > 0)
        make_endianless(buf, size);

    return ret_code;
}

/* Open for writing.  If `compress` is true we create a gzip stream. */
static inline int stream_open_write(const char *path, int compress, stream_t *out)
{
    if (compress)
    {
        out->mode = STREAM_GZIP;
        out->handle.gz = gzopen(path, "wb9"); /* max compression */
        if (!out->handle.gz)
        {
            LOG_ERROR("gzopen failed for %s", path);
            return -1;
        }
    }
    else
    {
        out->mode = STREAM_PLAIN;
        out->handle.plain = fopen(path, "wb");
        if (!out->handle.plain)
        {
            LOG_ERROR("fopen failed for %s", path);
            return -1;
        }
    }

    return 0;
}

/* Open for reading.  If `compress` is true we expect a gzip stream. */
static inline int stream_open_read(const char *path, int compress, stream_t *out)
{
    if (compress)
    {
        out->mode = STREAM_GZIP;
        out->handle.gz = gzopen(path, "rb");
        if (!out->handle.gz)
        {
            LOG_ERROR("gzopen failed for %s", path);
            return -1;
        }
    }
    else
    {
        out->mode = STREAM_PLAIN;
        out->handle.plain = fopen(path, "rb");
        if (!out->handle.plain)
        {
            LOG_ERROR("fopen failed for %s", path);
            return -1;
        }
    }

    return 0;
}

/* Close the abstract stream */
static inline void stream_close(stream_t *s)
{
    if (s->mode == STREAM_PLAIN)
    {
        if (s->handle.plain)
            fclose(s->handle.plain);
    }
    else
    {
        if (s->handle.gz)
            gzclose(s->handle.gz);
    }
}

static inline void stream_write_debug_meta(const char *var_name, size_t var_size, stream_t *s)
{
#ifdef FILE_SYSTEM_DEBUG
    LOG_DEBUG("Wrote \"%s\" of size %zu", var_name, var_size);
    u32 len = strlen(var_name) + 1;
    assert(stream_write(&len, sizeof(len), AS_STREAM(s)) == 0); // write length of name including null terminator
    assert(stream_write(&var_size, sizeof(var_size), AS_STREAM(s)) == 0);
    assert(stream_write(var_name, len, AS_STREAM(s)) == 0);
#endif
}

static inline void stream_read_debug_meta(char *expected_var_name, size_t expected_var_size, stream_t *s)
{
    // this not only reads the debug meta, but also discards it. Aborts if it doesn't match expected values.
#ifdef FILE_SYSTEM_DEBUG
    u32 var_len;
    u32 expected_var_len = (u32)(strlen(expected_var_name) + 1);

    assert(stream_read(&var_len, sizeof(var_len), AS_STREAM(s)) == 0);

    assert(var_len < 256);

    if (var_len != expected_var_len)
    {
        LOG_WARNING("Debug meta variable name length %u does not match expected length %u for variable %s", var_len,
                    expected_var_len, expected_var_name);
    }

    size_t var_size;
    assert(stream_read(&var_size, sizeof(var_size), AS_STREAM(s)) == 0);

    if (var_size != expected_var_size)
    {
        LOG_ERROR("Debug meta variable size %zu does not match expected size %zu for variable %s", var_size,
                    expected_var_size, expected_var_name);
                    abort();
    }

    char var_name[256];
    assert(stream_read(var_name, var_len, AS_STREAM(s)) == 0);
    var_name[var_len - 1] = '\0'; // ensure null termination

    if (strcmp(var_name, expected_var_name) != 0)
    {
        LOG_WARNING("Debug meta variable name \"%s\" does not match expected name \"%s\"", var_name, expected_var_name);
    }

    LOG_DEBUG("Read \"%s\" of size %zu", var_name, var_size);
#else
    (void)expected_var_name;
    (void)expected_var_size;
#endif
}

#define WRITE(object, s)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        stream_write_debug_meta(#object, sizeof(object), AS_STREAM(s));                                                \
        assert(stream_write(&(object), sizeof(object), AS_STREAM(s)) == 0);                                            \
    } while (0)

#define READ(object, s)                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        stream_read_debug_meta(#object, sizeof(object), AS_STREAM(s));                                                 \
        assert(stream_read(&(object), sizeof(object), AS_STREAM(s)) == 0);                                             \
    } while (0)

#define WRITE_N(ptr, s, size)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        stream_write_debug_meta(#ptr, size, AS_STREAM(s));                                                             \
        assert(stream_write((ptr), size, AS_STREAM(s)) == 0);                                                          \
    } while (0)

#define READ_N(ptr, s, size)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        stream_read_debug_meta(#ptr, size, AS_STREAM(s));                                                              \
        assert(stream_read((ptr), size, AS_STREAM(s)) == 0);                                                           \
    } while (0)

#define WRITE_OPTIONAL(condition, writer_block, stream)                                                                \
    {                                                                                                                  \
        u8 has_thing = (condition);                                                                                    \
        WRITE(has_thing, stream);                                                                                      \
        if (has_thing)                                                                                                 \
            writer_block                                                                                               \
    }

#define READ_OPTIONAL(reader_block, stream)                                                                            \
    {                                                                                                                  \
        u8 has_thing = 0;                                                                                              \
        READ(has_thing, stream);                                                                                       \
        if (has_thing)                                                                                                 \
            reader_block                                                                                               \
    }

#define WRITE_VEC(vec, iter, element, write_expr, stream)                                                              \
    {                                                                                                                  \
        i32 vec_length = (vec).length;                                                                                 \
        WRITE(vec_length, stream);                                                                                     \
        for (u32 iter = 0; iter < vec_length; iter++)                                                                  \
        {                                                                                                              \
            __auto_type element = (vec).data[iter];                                                                    \
            do                                                                                                         \
            {                                                                                                          \
                write_expr                                                                                             \
            } while (0);                                                                                               \
        }                                                                                                              \
    }

#define READ_VEC(vec, iter, element, read_expr, stream)                                                                \
    {                                                                                                                  \
        i32 vec_length = 0;                                                                                            \
        READ(vec_length, stream);                                                                                      \
        for (u32 iter = 0; iter < vec_length; iter++)                                                                  \
        {                                                                                                              \
            read_expr;                                                                                                 \
            (void)vec_push(&(vec), element);                                                                           \
        }                                                                                                              \
    }

void blob_write(blob b, stream_t *f);
blob blob_read(stream_t *f);

void blob_vars_write(blob b, stream_t *f);
blob blob_vars_read(stream_t *f);

void write_hashtable(hash_node **t, stream_t *f);
void read_hashtable(hash_node **t, stream_t *f);

void stream_embed_file_write(const char *file_path, stream_t *s);
void stream_embed_file_read_to_mem(stream_t *s, u8 **out_data, u32 *out_size);

u8 save_level(level lvl);
u8 load_level(level *lvl, const char *name);
// loads a level using an existing registry to acknolwedge block ids
u8 load_level_ack_registry(level *lvl, const char *name_in, block_registry *ack_reg);

#endif // FILE_SYSTEM_H
