#include "include/engine_events.h"

int is_user_event(int id)
{
    return (id >= SDL_USEREVENT && id < ENGINE_LAST_EVENT);
}

int is_block_event(int id)
{
    return (id > ENGINE_BLOCK_SECTION_START && id < ENGINE_BLOCK_SECTION_END);
}