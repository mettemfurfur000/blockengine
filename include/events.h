#ifndef EVENTS_H
#define EVENTS_H

#include "flags.h"
#include "general.h"

#include "SDL2/SDL.h"

typedef enum
{
    ENGINE_BLOCK_SECTION_START = SDL_USEREVENT,

    ENGINE_BLOCK_UDPATE,
    ENGINE_BLOCK_ERASED,
    ENGINE_BLOCK_SET,
    ENGINE_BLOCK_MOVE,

    ENGINE_BLOCK_SECTION_END,

    ENGINE_BLOB_UPDATE,

    ENGINE_LAST_EVENT
} ENGINE_EVENTS;

#define TOTAL_ENGINE_EVENTS ENGINE_LAST_EVENT - SDL_USEREVENT

typedef struct
{
    Uint32 type; // kind of standart header for event structure
    Uint32 timestamp;
    // 8
    u64 target_id;
    // 16
    Uint32 target_x;
    Uint32 target_y;
    // 24
    Uint16 target_layer_id;
    Uint16 previous_id;
    Uint16 new_id;
    // 30
} block_update_event;

typedef struct
{
    Uint32 type; // kind of standart header for event structure
    Uint32 timestamp;
    // 8
    u64 target_id;
    // 16
    Uint32 target_x;
    Uint32 target_y;
    // 24
    Uint16 target_layer_id;
    char letter;
    int size_change;
    int size;
    u8 *element_value;
    // 28
} blob_update_event;

typedef struct
{
    Uint32 type; // kind of standart header for event structure
    Uint32 timestamp;
    // 8
    u64 target_id;
    u64 actor_id;
    // 24
    Uint32 target_x;
    Uint32 target_y;
    // 32
    Uint32 actor_x;
    Uint32 actor_y;
    // 40
    Uint16 target_layer_id;
    Uint16 actor_layer_id;
    // 44
    u8 blob_letter; // for blob updates
    u8 reserve_flags;
    // 46
    u8 signal[10];
    // 56
} engine_event;

#define IS_ENGINE_EVENT(id) (id >= SDL_USEREVENT && id < ENGINE_LAST_EVENT)
#define IS_BLOCK_EVENT(id) (id > ENGINE_BLOCK_SECTION_START && id < ENGINE_BLOCK_SECTION_END)

#endif