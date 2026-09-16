#ifndef SCRIPTING_BLOCK_COMPONENT_H
#define SCRIPTING_BLOCK_COMPONENT_H 1

#include <lua.h>

// registers component-related globals so component scripts can mutate the
// currently-loading block resource and register component APIs
void lua_block_component_register(lua_State *L);

#endif