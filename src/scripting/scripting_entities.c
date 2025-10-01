// #include "include/entity.h"
// #include "include/level.h"
// #include "include/scripting.h"
// #include "include/scripting_bindings.h"

// #include <lauxlib.h>
// #include <lua.h>

// /* Entity userdata wrapper (stores the owning entity_table pointer + packed handle u32)
//    Similar pattern to VarHandleUser for stable Lua userdata. */
// typedef struct EntityWrapper
// {
//     entity_table *t;
//     u32 packed; /* u32 representation of handle32 */
// } EntityWrapper;

// static int push_entity(lua_State *L, entity_table *t, handle32 h)
// {
//     EntityWrapper *eu = (EntityWrapper *)lua_newuserdata(L, sizeof(EntityWrapper));
//     eu->t = t ? t : g_entity_table;
//     eu->packed = handle_to_u32(h);
//     luaL_getmetatable(L, "Entity");
//     lua_setmetatable(L, -2);
//     return 1;
// }

// /* Helper to fetch entity pointer and handle from userdata at idx. Returns NULL for invalid. */
// static entity *get_entity_from_userdata(lua_State *L, int idx, entity_table **out_table, handle32 *out_h)
// {
//     EntityWrapper *eu = (EntityWrapper *)luaL_checkudata(L, idx, "Entity");
//     if (!eu)
//         return NULL;
//     entity_table *t = eu->t ? eu->t : g_entity_table;
//     handle32 h = handle_from_u32(eu->packed);
//     if (out_table)
//         *out_table = t;
//     if (out_h)
//         *out_h = h;
//     if (!t)
//         return NULL;
//     return entity_table_get(t, h);
// }

// static int lua_entity_is_valid(lua_State *L)
// {
//     EntityWrapper *eu = (EntityWrapper *)luaL_checkudata(L, 1, "Entity");
//     if (!eu || !eu->t)
//     {
//         lua_pushboolean(L, 0);
//         return 1;
//     }
//     handle32 h = handle_from_u32(eu->packed);
//     lua_pushboolean(L, handle_is_valid(eu->t->handles, h));
//     return 1;
// }

// static int lua_entity_uuid(lua_State *L)
// {
//     entity_table *t;
//     handle32 h;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//     {
//         lua_pushnil(L);
//         return 1;
//     }
//     lua_pushinteger(L, e->uuid);
//     return 1;
// }

// static int lua_entity_get_name(lua_State *L)
// {
//     entity_table *t;
//     handle32 h;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//     {
//         lua_pushnil(L);
//         return 1;
//     }
//     lua_pushstring(L, e->name ? e->name : "");
//     return 1;
// }

// static int lua_entity_get_room(lua_State *L)
// {
//     entity_table *t;
//     handle32 h;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e || e->type != ENTITY_TYPE_FULL)
//     {
//         lua_pushnil(L);
//         return 1;
//     }

//     /* prefer explicit parent_room pointer*/
//     if (e->payload.full.parent_room)
//     {
//         LOG_ERROR("Entity is not placed in a room with a physics world");
//         lua_pushnil(L);
//         return 1;
//     }

//     NEW_USER_OBJECT(L, Room, e->payload.full.parent_room);
//     return 1;
// }

// static int lua_entity_get_position(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//     {
//         lua_pushnil(L);
//         return 1;
//     }

//     physics_body_t pb = NULL;
//     if (e->type == ENTITY_TYPE_FULL)
//         pb = e->payload.full.body_phys;
//     else if (e->type == ENTITY_TYPE_POINT)
//         pb = e->payload.point.body;

//     if (!pb)
//     {
//         lua_pushnil(L);
//         return 1;
//     }

//     b2Vec2 pos = {0};
//     physics_body_get_position(pb, &pos);
//     lua_pushnumber(L, pos.x);
//     lua_pushnumber(L, pos.y);
//     return 2;
// }

// static int lua_entity_set_position(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//         return luaL_error(L, "Invalid Entity");

//     float x = (float)luaL_checknumber(L, 2);
//     float y = (float)luaL_checknumber(L, 3);

//     physics_body_t pb = NULL;
//     if (e->type == ENTITY_TYPE_FULL)
//         pb = e->payload.full.body_phys;
//     else if (e->type == ENTITY_TYPE_POINT)
//         pb = e->payload.point.body;

//     if (!pb)
//         return luaL_error(L, "Entity has no physics body");

//     b2Vec2 pos = {x, y};
//     physics_body_set_position(pb, pos);
//     return 0;
// }

// static int lua_entity_get_angle(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//     {
//         lua_pushnil(L);
//         return 1;
//     }
//     physics_body_t pb = (e->type == ENTITY_TYPE_FULL) ? e->payload.full.body_phys : e->payload.point.body;
//     if (!pb)
//     {
//         lua_pushnil(L);
//         return 1;
//     }
//     float a = physics_body_get_angle(pb);
//     lua_pushnumber(L, a);
//     return 1;
// }

// static int lua_entity_set_angle(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//         return luaL_error(L, "Invalid Entity");

//     float a = (float)luaL_checknumber(L, 2);
//     physics_body_t pb = (e->type == ENTITY_TYPE_FULL) ? e->payload.full.body_phys : e->payload.point.body;
//     if (!pb)
//         return luaL_error(L, "Entity has no physics body");
//     physics_body_set_angle(pb, a);
//     return 0;
// }

// static int lua_entity_get_velocity(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//     {
//         lua_pushnil(L);
//         return 1;
//     }
//     physics_body_t pb = (e->type == ENTITY_TYPE_FULL) ? e->payload.full.body_phys : e->payload.point.body;
//     if (!pb)
//     {
//         lua_pushnil(L);
//         return 1;
//     }
//     b2Vec2 vel = {0};
//     physics_body_get_linear_velocity(pb, &vel);
//     lua_pushnumber(L, vel.x);
//     lua_pushnumber(L, vel.y);
//     return 2;
// }

// static int lua_entity_set_velocity(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//         return luaL_error(L, "Invalid Entity");
//     float vx = (float)luaL_checknumber(L, 2);
//     float vy = (float)luaL_checknumber(L, 3);
//     physics_body_t pb = (e->type == ENTITY_TYPE_FULL) ? e->payload.full.body_phys : e->payload.point.body;
//     if (!pb)
//         return luaL_error(L, "Entity has no physics body");
//     b2Vec2 v = {vx, vy};
//     physics_body_set_linear_velocity(pb, v);
//     return 0;
// }

// static int lua_entity_apply_force(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e)
//         return luaL_error(L, "Invalid Entity");
//     float fx = (float)luaL_checknumber(L, 2);
//     float fy = (float)luaL_checknumber(L, 3);
//     physics_body_t pb = (e->type == ENTITY_TYPE_FULL) ? e->payload.full.body_phys : e->payload.point.body;
//     if (!pb)
//         return luaL_error(L, "Entity has no physics body");
//     b2Vec2 f = {fx, fy};
//     physics_body_apply_force(pb, f);
//     return 0;
// }

// static int lua_entity_get_handle(lua_State *L)
// {
//     EntityWrapper *eu = (EntityWrapper *)luaL_checkudata(L, 1, "Entity");
//     if (!eu)
//     {
//         lua_pushnil(L);
//         return 1;
//     }
//     lua_pushinteger(L, eu->packed);
//     return 1;
// }

// static int lua_entity_attach(lua_State *L)
// {
//     /* self: Entity, child: Entity */
//     entity_table *t_parent = NULL;
//     handle32 parent_h = INVALID_HANDLE;
//     entity *parent = get_entity_from_userdata(L, 1, &t_parent, &parent_h);
//     if (!parent || !t_parent)
//         return luaL_error(L, "Invalid parent Entity");

//     /* accept either Entity userdata or integer handle for child */
//     handle32 child_h = INVALID_HANDLE;
//     entity_table *t_child = t_parent;
//     if (luaL_testudata(L, 2, "Entity"))
//     {
//         EntityWrapper *ceu = (EntityWrapper *)luaL_checkudata(L, 2, "Entity");
//         child_h = handle_from_u32(ceu->packed);
//         t_child = ceu->t ? ceu->t : g_entity_table;
//     }
//     else if (lua_isinteger(L, 2))
//     {
//         u32 v = (u32)lua_tointeger(L, 2);
//         child_h = handle_from_u32(v);
//         t_child = t_parent;
//     }
//     else
//     {
//         return luaL_error(L, "Invalid argument for child entity");
//     }

//     if (t_child != t_parent)
//     {
//         return luaL_error(L, "Cannot attach entities from different tables");
//     }

//     lua_pushboolean(L, entity_attach(t_parent, parent_h, child_h));
//     return 1;
// }

// static int lua_entity_detach(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e || !t)
//         return luaL_error(L, "Invalid Entity");

//     lua_pushboolean(L, entity_detach(t, h));
//     return 1;
// }

// static int lua_entity_children(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     entity *e = get_entity_from_userdata(L, 1, &t, &h);
//     if (!e || !t)
//     {
//         lua_pushnil(L);
//         return 1;
//     }

//     lua_newtable(L);
//     for (int i = 0; i < e->children.length; ++i)
//     {
//         handle32 ch = e->children.data[i];
//         push_entity(L, t, ch);
//         lua_seti(L, -2, i + 1);
//     }
//     return 1;
// }

// static int lua_entity_release(lua_State *L)
// {
//     entity_table *t = NULL;
//     handle32 h = INVALID_HANDLE;
//     (void)get_entity_from_userdata(L, 1, &t, &h);
//     if (!t)
//     {
//         lua_pushboolean(L, 0);
//         return 1;
//     }
//     entity_table_release(t, h);
//     lua_pushboolean(L, 1);
//     return 1;
// }

// /* wrap a numeric handle (u32) into Entity userdata */
// static int lua_entity_wrap(lua_State *L)
// {
//     u32 h = (u32)luaL_checkinteger(L, 1);
//     EntityWrapper *eu = (EntityWrapper *)lua_newuserdata(L, sizeof(EntityWrapper));
//     eu->t = g_entity_table;
//     eu->packed = h;
//     luaL_getmetatable(L, "Entity");
//     lua_setmetatable(L, -2);
//     return 1;
// }

// // Create a point entity: create_point(name, x, y) -> handle (unsigned int)
// static int lua_create_point_entity(lua_State *L)
// {
//     const char *name = luaL_checkstring(L, 1);
//     int x = luaL_checkinteger(L, 2);
//     int y = luaL_checkinteger(L, 3);

//     b2Vec2 pos = {.x = (float)x, .y = (float)y};
//     handle32 h = entity_create_point(NULL, name, pos);

//     lua_pushinteger(L, (lua_Integer)handle_to_u32(h));
//     return 1;
// }

// // Create a full entity: create_full(name, room, collision_mask, layer_index) -> handle
// static int lua_create_full_entity(lua_State *L)
// {
//     const char *name = luaL_checkstring(L, 1);
//     LUA_CHECK_USER_OBJECT(L, Room, wrapper, 2);
//     u64 mask = (u64)luaL_checkinteger(L, 3);
//     u32 layer_index = (u32)luaL_checkinteger(L, 4);
//     LUA_CHECK_USER_OBJECT(L, Room, parent_wrapper, 5);

//     room *parent_rptr = parent_wrapper->r;
//     room *r = wrapper->r;

//     /* pass room by value and use global table; allow optional layer index and optional parent room */
//     handle32 h = entity_create_full(NULL, name, *r, parent_rptr, layer_index, mask);
//     if (!handle_is_valid(NULL, h))
//     {
//         const char *pname = parent_rptr && parent_rptr->name ? parent_rptr->name : (r->name ? r->name : "<unnamed>");
//         luaL_error(L, "entity.create_full failed: physics body creation failed for room '%s' layer %u", pname,
//                    layer_index);
//         return 0;
//     }

//     lua_pushinteger(L, (lua_Integer)handle_to_u32(h));
//     return 1;
// }

// // simple Room:step_physics(dt)
// static int lua_room_step_physics(lua_State *L)
// {
//     LUA_CHECK_USER_OBJECT(L, Room, wrapper, 1);
//     double dt = luaL_checknumber(L, 2);

//     room *r = wrapper->r;
//     if (r && r->physics_world)
//     {
//         physics_world_step(r->physics_world, (float)dt);
//     }

//     return 0;
// }

// void lua_entity_register(lua_State *L)
// {
//     const static luaL_Reg ent_lib[] = {
//         {"create_point", lua_create_point_entity},
//         { "create_full",  lua_create_full_entity},
//         {        "wrap",         lua_entity_wrap}, /* wrap a numeric handle into Entity userdata */
//         {          NULL,                    NULL}
//     };

//     // library
//     luaL_newlib(L, ent_lib);
//     lua_setglobal(L, "entity");

//     // attach Room:step_physics
//     luaL_getmetatable(L, "Room");
//     lua_pushcfunction(L, lua_room_step_physics);
//     lua_setfield(L, -2, "step_physics");
//     lua_pop(L, 1);

//     /* Create Entity metatable */
//     const static luaL_Reg entity_methods[] = {
//         {    "is_valid",     lua_entity_is_valid},
//         {        "uuid",         lua_entity_uuid},
//         {        "name",     lua_entity_get_name},
//         {        "room",     lua_entity_get_room},
//         {      "attach",       lua_entity_attach},
//         {      "detach",       lua_entity_detach},
//         {    "children",     lua_entity_children},
//         {     "release",      lua_entity_release},
//         {      "handle",   lua_entity_get_handle},
//         {"get_position", lua_entity_get_position},
//         {"set_position", lua_entity_set_position},
//         {   "get_angle",    lua_entity_get_angle},
//         {   "set_angle",    lua_entity_set_angle},
//         {"get_velocity", lua_entity_get_velocity},
//         {"set_velocity", lua_entity_set_velocity},
//         { "apply_force",  lua_entity_apply_force},
//         {          NULL,                    NULL}
//     };

//     luaL_newmetatable(L, "Entity");
//     lua_pushvalue(L, -1);
//     lua_setfield(L, -2, "__index");
//     luaL_setfuncs(L, entity_methods, 0);
//     lua_pop(L, 1);
// }
