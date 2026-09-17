#ifndef SCRIPTING_BLOCK_TRAIT_H
#define SCRIPTING_BLOCK_TRAIT_H 1

#include <lua.h>

// registers trait-related globals so trait scripts can mutate the currently
// loading block resource and register trait APIs
void lua_block_trait_register(lua_State *L);

#endif