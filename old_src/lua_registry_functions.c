#include "include/lua_registry_functions.h"

int lua_read_registry_entry(lua_State *L)
{
    scripting_check_arguments(L, 2, LUA_TLIGHTUSERDATA, LUA_TNUMBER);

    block_resources_t *reg = (block_resources_t *)lua_touserdata(L, 1);
    int block_id = lua_tonumber(L, 2);

    for (int i = 0; i < reg->length; i++)
    {
        if (reg->data[i].block_sample.id == block_id)
        {
            hash_table **src_table = reg->data[i].all_fields;

            lua_newtable(L);

            hash_table *node;
            hash_table *next_node;

            for (int i = 0; i < TABLE_SIZE; ++i)
            {
                node = src_table[i];
                while (node != NULL)
                {
                    next_node = node->next;
                    lua_pushstring(L, node->key);
                    lua_pushstring(L, node->value);
                    lua_settable(L, -3);
                    // printf("    %s %s %s\n", node->key, strlen(node->value) ? "=" : "", node->value);
                    node = next_node;
                }
            }

            return 1; /* number of results */
        }
    }

    lua_pushnil(L);
    return 1; /* number of results */
}