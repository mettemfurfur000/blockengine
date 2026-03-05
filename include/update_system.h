#include "data_io.h"
#ifndef LEVEL_UPDATE_SYSTEM_H
#define LEVEL_UPDATE_SYSTEM_H 1

#include "general.h"
// #include "vec/src/vec.h"

#include "handle.h"
#include "hashtable.h"
#include <assert.h>

// typedef struct
// {
//     handle32 var_handle;
//     u8 length;
//     u8 bytes[256];
// } update_var;

// typedef vec_t(update_var) update_var_t;

// sinc updates can happen out of order to de elements of our massive arrays of both blocks and vars,
// we have to hold a big accumulator to store all the updates in here

// which is not that bad i think... for var updates to reach 1 mb of space you would have to
// update 4k vars in 1 tick

// even if it does reach 1 mb of space in ram, it still will be cut down to de amount of actual block var changes,
// if its only 1 var change in all 4k vars that would be
// handle32, 4 bytes + 1 for size byte + 1 var offset + 1 size byte + 1-255 bytes of payload, 7 * 4k = 27 kb per second

// block updates are much smaller compared to vars, each block is 2 + 2 + 1-8 = 5-12 bytes worth, which is
// 200k block updates per second to take 1mb/s with 1 byte sized blocks, or 87k updates for 8 byte blocks

// which can be optimized further, if each block of a layer is updated every frame, we can record the whole
// layer byte by byte and apply some fast compression to it, then it will only depend on how random the updates are

// when set to 1 the update system should be inactive

typedef struct
{ // struct that is decoded from the update
    u64 id;
    u16 x;
    u16 y;
} update_block;

typedef struct
{
    handle32 h;
    u16 x;
    u16 y;
} update_varhandle;

typedef enum
{
    COMPONENT_UPDATE_NEW,    // new var created
    COMPONENT_UPDATE_SET,    // singular component updated
    COMPONENT_UPDATE_ADD,    // singular component appended
    COMPONENT_UPDATE_DELETE, // component deleted
    COMPONENT_UPDATE_RESIZE, // component resized
    COMPONENT_UPDATE_RENAME, // component renamed
} update_component_type;

typedef struct
{
    union
    {
        blob* blob;
        u8 *raw;
    };
    handle32 h;
    u16 x;
    u16 y;
    char letter;
    char new_char;
    u8 size;
    update_component_type type;
} update_var_component;

typedef struct
{
    stream_t update_stream;
    u32 update_count; // amount of packets recorded
} update_accumulator;

update_accumulator update_acc_new();
void update_acc_free(update_accumulator *a);

// x y for pos, id for value, true_width for width of the value in bytes
void update_block_push(update_accumulator *a, u16 x, u16 y, u64 id, u8 true_width);
update_block update_block_read(update_accumulator a, u8 true_width);

void update_var_push(update_accumulator *a, u16 x, u16 y, handle32 handle);
update_varhandle update_var_read(update_accumulator a);

void update_component_new_push(update_accumulator *a, handle32 handle, blob b);
void update_component_set_push(update_accumulator *a, handle32 handle, char c, u8 len, u8 *ptr);

#define UPDATE_COMP_SET_GEN_NAME(T)                                                                                    \
    void update_component_set_push_##T(update_accumulator *a, handle32 handle, char c, T value);

UPDATE_COMP_SET_GEN_NAME(u8)
UPDATE_COMP_SET_GEN_NAME(u16)
UPDATE_COMP_SET_GEN_NAME(u32)
UPDATE_COMP_SET_GEN_NAME(u64)

UPDATE_COMP_SET_GEN_NAME(i8)
UPDATE_COMP_SET_GEN_NAME(i16)
UPDATE_COMP_SET_GEN_NAME(i32)
UPDATE_COMP_SET_GEN_NAME(i64)

void update_component_add_push(update_accumulator *a, handle32 handle, char c, u8 size);
void update_component_delete_push(update_accumulator *a, handle32 handle, char c);
void update_component_resize_push(update_accumulator *a, handle32 handle, char c, u8 new_size);
void update_component_rename_push(update_accumulator *a, handle32 handle, char c, char new_name);

update_var_component update_component_read(update_accumulator a);

#endif