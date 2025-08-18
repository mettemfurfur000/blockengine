#ifndef SCRIPTING_VAR_HANDLES_H
#define SCRIPTING_VAR_HANDLES_H

#include "level.h"
#include "level.h"

/* VarHandle helpers (defined in scripting_var_handles.c) */
void lua_varhandle_register(lua_State *L);
int push_varhandle(lua_State *L, layer *l, u32 packed);
blob *get_blob_from_varhandle(lua_State *L, int idx);

#endif // SCRIPTING_VAR_HANDLES_H