#ifndef SCRIPTING_VAR_HANDLES_H
#define SCRIPTING_VAR_HANDLES_H

#include "level.h"

/* VarHandle helpers (defined in scripting_var_handles.c) */
int push_varhandle(lua_State *L, layer *l, handle32 handle);

void lua_varhandle_register(lua_State *L);
int push_varhandle(lua_State *L, layer *l, handle32 handle);
blob *get_blob_from_varhandle(lua_State *L, int idx);

layer *get_layer_from_varhandle(lua_State *L, int idx);
handle32 get_handle_from_varhandle(lua_State *L, int idx);

#endif // SCRIPTING_VAR_HANDLES_H