#ifndef ENTITY_H
#define ENTITY_H

#include "hashtable.h"
#include "level.h"

// TODO: write an entity renderer shader stuff thing

typedef struct
{
    room body;
    u64 parent_entity;

    float pos_x; // position of de center of de room
    float pos_y;
    float pos_z;    // elevation abov the ground, for shadows and stuf
    float rotation; // degrees

    u64 parented[16]; // ??
    u8 parented_num;
    u8 flags;
    // ??
} entity;

typedef struct
{
    hash_node **entity_table;
} entity_storage;

// attempts to access said entity
// NULL if not found
entity *get_entity(entity_storage *s, u64 uuid);
// adds said entity to the storage based on its room uuid
// true on success
bool add_entity(entity_storage *s, entity *e);
// removes said entity if room uuid matches
// true on success
bool remove_entity(entity_storage *s, u64 uuid);

#endif