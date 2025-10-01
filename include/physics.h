/* C wrapper for physics (Box2D) functionality. Implemented in C++.
   The API is intentionally minimal and opaque so C code can use it. */
#ifndef PHYSICS_H
#define PHYSICS_H

// #include "general.h"
// #include <box2d/box2d.h>

// /* Opaque world handle */
// typedef void *physics_world_t;
// /* Opaque body handle */
// typedef void *physics_body_t;
// /* forward declare room used in API */
// struct room;
// /* Opaque joint handle */
// typedef void *physics_joint_t;

// /* Create / destroy a top-down world (gravity defaults to 0,0). Returns NULL on failure. */
// physics_world_t physics_world_create();
// void physics_world_destroy(physics_world_t w);

// /* Step the world by dt seconds (fixed iteration counts are fine for now). */
// void physics_world_step(physics_world_t w, float dt);

// /* Create/destroy a body for a full entity.
//    The implementation should build fixtures based on the provided room and layer_index.
//    On failure (for example, detached / non-rectangular regions) returns NULL.
//    `entity_ptr` is stored as user data on the created body. */
// physics_body_t physics_create_body_for_full_entity(physics_world_t w, struct room *r, unsigned layer_index, void *entity_ptr, u64 collision_layer_mask);
// void physics_destroy_body(physics_world_t w, physics_body_t b);

// /* Body accessors: position (b2Vec2), angle (radians), linear velocity (b2Vec2).
//    These are simple wrappers around Box2D body getters/setters. */
// void physics_body_get_position(physics_body_t b, b2Vec2 *out_pos);
// void physics_body_set_position(physics_body_t b, b2Vec2 pos);
// float physics_body_get_angle(physics_body_t b);
// void physics_body_set_angle(physics_body_t b, float a);
// void physics_body_get_linear_velocity(physics_body_t b, b2Vec2 *out_vel);
// void physics_body_set_linear_velocity(physics_body_t b, b2Vec2 vel);
// /* apply impulse/force at center */
// void physics_body_apply_force(physics_body_t b, b2Vec2 force);

// /* Create / destroy a simple weld joint between two bodies. Returns NULL on failure. */
// physics_joint_t physics_create_weld_joint(physics_world_t w, physics_body_t a, physics_body_t b);
// void physics_destroy_joint(physics_world_t w, physics_joint_t j);

#endif
