#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include "../include/general.h"

#define obj_pool_node(T) \
  struct { T object; u8 active; }

#define obj_pool_t(T)\
  struct { T objects; u32 inactive_nodes; }

#endif