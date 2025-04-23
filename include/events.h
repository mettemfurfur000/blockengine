#ifndef EVENTS_H
#define EVENTS_H 1

// #include "flags.h"
#include "general.h"

#include <SDL2/SDL.h>

typedef enum ENGINE_EVENTS
{
    ENGINE_BLOCK_SECTION_START = SDL_USEREVENT,

    ENGINE_BLOCK_UDPATE,
    ENGINE_BLOCK_ERASED,
    ENGINE_BLOCK_CREATE,

    ENGINE_BLOCK_SECTION_END,

    ENGINE_SPECIAL_SIGNAL,

    ENGINE_SPECIAL_END,

    ENGINE_BLOB_UPDATE,
    ENGINE_BLOB_ERASED,
    ENGINE_BLOB_CREATE,

    ENGINE_BLOB_SECTION_END,

    ENGINE_TICK,
    ENGINE_INIT,

    ENGINE_LAST_EVENT
} ENGINE_EVENTS;

#define TOTAL_ENGINE_EVENTS ENGINE_LAST_EVENT - SDL_USEREVENT

typedef struct block_update_event
{
    Uint32 type;
    Uint32 timestamp;

    void *room_ptr;
    void *layer_ptr;
    u64 new_id;
    u64 previous_id;

    u32 x;
    u32 y;
} block_update_event;

static_assert(sizeof(block_update_event) <= sizeof(SDL_Event), "");

typedef struct blob_update_event
{
    Uint32 type; // kind of standart header for event structure
    Uint32 timestamp;

    void *room_ptr;
    void *layer_ptr;
    u64 new_id;

    u32 x;
    u32 y;

    u16 pos;
    char letter;
    int size;
    u8 *ptr;
    // 28
} blob_update_event;

static_assert(sizeof(blob_update_event) <= sizeof(SDL_Event), "");

// typedef struct special_event
// {
//     Uint32 type;
//     Uint32 timestamp;

//     void *room_ptr;
//     void *layer_ptr;

//     const u8* bytes;
//     u32 length;

//     u32 x;
//     u32 y;

// } special_event;

// static_assert(sizeof(special_event) <= sizeof(SDL_Event));

#define IS_ENGINE_EVENT(id) (id >= SDL_USEREVENT && id < ENGINE_LAST_EVENT)
#define IS_BLOCK_EVENT(id)                                                     \
    (id > ENGINE_BLOCK_SECTION_START && id < ENGINE_BLOCK_SECTION_END)

#endif