#ifndef ENGINE_EVENTS_H
#define ENGINE_EVENTS_H

#include "flags.h"
#include "engine_types.h"

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
    block *target;
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
    block *target;
    // 16
    Uint32 target_x;
    Uint32 target_y;
    // 24
    Uint16 target_layer_id;
    char letter;
    int size_change;
    int size;
    byte *element_value;
    // 28
} blob_update_event;

typedef struct
{
    Uint32 type; // kind of standart header for event structure
    Uint32 timestamp;
    // 8
    block *target;
    block *actor;
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
    byte blob_letter; // for blob updates
    byte reserve_flags;
    // 46
    byte signal[10];
    // 56
} engine_event;

int is_user_event(int id);
int is_block_event(int id);

#endif