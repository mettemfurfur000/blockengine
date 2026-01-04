#ifndef LEVEL_UPDATE_SYSTEM_H
#define LEVEL_UPDATE_SYSTEM_H 1

#include "vec/src/vec.h"
// #include "level.h"
#include "../vec/src/vec.h"
#include "general.h"
#include "handle.h"
#include <assert.h>

typedef struct { // struct that is decoded from the update
    u16 x;
    u16 y;
    u64 id;
} update_block;

typedef struct
{
    handle32 var_handle;
    u8 length;
    u8 bytes[256];
} update_var;

typedef vec_t(update_var) update_var_t;

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

typedef vec_t(u8) vec_i8_t;

typedef struct {
    vec_i8_t block_updates_raw;
    // update_var_t vars; // TODO: do variables
} update_acc;

// typedef struct
// {
//     u16 room_index;
//     u16 layer_index;
// } update_header;

// typedef enum {
//     UPDATE_BLOCKS,
//     UPDATE_VARS
// } update_types;

// #define UPDATE_MAX_BYTES 4096

// void update_start_buffer(u8* dest, u8 type, update_header upd);
// void update_push_block(u8* dest, update_block blk);

#endif