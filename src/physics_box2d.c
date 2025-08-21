#include "../include/physics.h"

// Minimal Box2D wrapper implementation using the C-style thin API present in
// MSYS2's Box2D headers (functions like b2CreateWorld, b2World_Step, b2CreateBody).
#include "../include/level.h"
#include "../include/logging.h"
#include "../include/physics.h"

#include <box2d/box2d.h>
#include <math.h>
#include <stdlib.h>

typedef struct
{
    b2WorldId id;
} PhysicsWorldWrapper;

typedef struct
{
    b2BodyId id;
    // store shape ids in a small dynamic array if needed; for now single shape
    b2ShapeId shape;
} PhysicsBodyWrapper;

typedef struct
{
    b2JointId id;
} PhysicsJointWrapper;

physics_world_t physics_world_create()
{
    // Create a world with default definition (zero gravity)
    b2WorldDef def = b2DefaultWorldDef();
    def.gravity = (b2Vec2){0.0f, 0.0f};

    PhysicsWorldWrapper *w = (PhysicsWorldWrapper *)malloc(sizeof(PhysicsWorldWrapper));
    if (!w)
        return NULL;
    w->id = b2CreateWorld(&def);
    return (physics_world_t)w;
}

void physics_world_destroy(physics_world_t w)
{
    if (!w)
        return;
    PhysicsWorldWrapper *pw = (PhysicsWorldWrapper *)w;
    if (b2World_IsValid(pw->id))
        b2DestroyWorld(pw->id);
    free(pw);
}

void physics_world_step(physics_world_t w, float dt)
{
    if (!w)
        return;
    PhysicsWorldWrapper *pw = (PhysicsWorldWrapper *)w;
    if (!b2World_IsValid(pw->id))
        return;
    // use a reasonable substep count (4)
    b2World_Step(pw->id, dt, 4);
}

// naive rectangle merge: scans the given layer and builds one or more axis-aligned boxes
// for contiguous filled tiles. If detached blocks are detected (holes inside regions) we
// currently fail and return NULL. This is a simple implementation that can be improved.
physics_body_t physics_create_body_for_full_entity(physics_world_t w, struct room *r, unsigned layer_index,
                                                   void *entity_ptr, u64 collision_layer_mask)
{
    if (!w)
        return NULL;
    PhysicsWorldWrapper *pw = (PhysicsWorldWrapper *)w;
    if (!b2World_IsValid(pw->id))
        return NULL;

    if (!r)
        return NULL;
    if ((int)layer_index >= r->layers.length)
        return NULL;

    void *layer_ptr = r->layers.data[layer_index];
    if (!layer_ptr)
        return NULL;
    layer *lay = (layer *)layer_ptr;

    // simple scan: treat any non-zero block id as solid. Build rectangles by scanning
    // horizontally and consuming runs. This will produce a set of AABB boxes.
    const u32 W = lay->width;
    const u32 H = lay->height;
    u8 *blocks = lay->blocks;
    u32 stride = lay->total_bytes_per_block;

    // validate layer for detached blocks / holes before creating body
    for (u32 y2 = 0; y2 < H; ++y2)
    {
        for (u32 x2 = 0; x2 < W; ++x2)
        {
            u64 cur = 0;
            memcpy(&cur, blocks + (y2 * W + x2) * stride, lay->block_size);

            // examine 4 neighbors (treat out-of-bounds as empty/non-solid)
            int solid_neighbors = 0;
            u32 nx, ny;

            // up
            nx = x2;
            ny = (y2 == 0) ? (u32)-1 : y2 - 1;
            if (y2 != 0)
            {
                u64 nid = 0;
                memcpy(&nid, blocks + (ny * W + nx) * stride, lay->block_size);
                if (nid != 0)
                    ++solid_neighbors;
            }
            // down
            if (y2 + 1 < H)
            {
                u64 nid = 0;
                memcpy(&nid, blocks + ((y2 + 1) * W + x2) * stride, lay->block_size);
                if (nid != 0)
                    ++solid_neighbors;
            }
            // left
            if (x2 != 0)
            {
                u64 nid = 0;
                memcpy(&nid, blocks + (y2 * W + (x2 - 1)) * stride, lay->block_size);
                if (nid != 0)
                    ++solid_neighbors;
            }
            // right
            if (x2 + 1 < W)
            {
                u64 nid = 0;
                memcpy(&nid, blocks + (y2 * W + (x2 + 1)) * stride, lay->block_size);
                if (nid != 0)
                    ++solid_neighbors;
            }

            // Rule 1: solid tile with zero solid neighbors -> detached isolated block
            if (cur != 0 && solid_neighbors == 0)
            {
                LOG_ERROR("Detached/isolated block detected in room '%s' layer %u at %u,%u (id=%llu)",
                          r->name ? r->name : "<unnamed>", layer_index, (unsigned)x2, (unsigned)y2,
                          (unsigned long long)cur);
                return NULL;
            }

            // Rule 2: empty tile with 4 solid neighbors -> hole inside shape
            if (cur == 0 && solid_neighbors == 4)
            {
                LOG_ERROR("Hole (empty tile surrounded) detected in room '%s' layer %u at %u,%u",
                          r->name ? r->name : "<unnamed>", layer_index, (unsigned)x2, (unsigned)y2);
                return NULL;
            }
        }
    }

    // temporary visited map
    unsigned *visited = (unsigned *)calloc(W * H, sizeof(unsigned));
    if (!visited)
        return NULL;

    // create body
    b2BodyDef bd = b2DefaultBodyDef();
    bd.type = b2_dynamicBody;
    bd.position = b2Vec2_zero;
    b2BodyId body = b2CreateBody(pw->id, &bd);

    b2ShapeDef shapeDef = {};
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.3f;
    shapeDef.isSensor = false;

    int created_shapes = 0;
    for (u32 y = 0; y < H; ++y)
    {
        for (u32 x = 0; x < W; ++x)
        {
            unsigned idx = y * W + x;
            if (visited[idx])
                continue;
            // get block id
            u64 id = 0;
            memcpy(&id, blocks + idx * stride, lay->block_size);
            if (id == 0)
                continue;

            // find horizontal run length
            u32 run_x = x;
            while (run_x < W)
            {
                u64 id2 = 0;
                memcpy(&id2, blocks + (y * W + run_x) * stride, lay->block_size);
                if (id2 == 0)
                    break;
                ++run_x;
            }
            u32 run_len = run_x - x;

            // try to extend vertically as long as every column in [x, x+run_len) has non-zero id
            u32 end_y = y + 1;
            for (;;)
            {
                if (end_y >= H)
                    break;
                bool ok = true;
                for (u32 xx = x; xx < x + run_len; ++xx)
                {
                    u64 id3 = 0;
                    memcpy(&id3, blocks + (end_y * W + xx) * stride, lay->block_size);
                    if (id3 == 0)
                    {
                        ok = false;
                        break;
                    }
                }
                if (!ok)
                    break;
                ++end_y;
            }

            // mark visited
            for (u32 yy = y; yy < end_y; ++yy)
                for (u32 xx = x; xx < x + run_len; ++xx)
                    visited[yy * W + xx] = 1;

            // create box for this rectangle. Convert tile units to world units (1 tile = 1 meter)
            float half_w = (float)run_len * 0.5f;
            float half_h = (float)(end_y - y) * 0.5f;
            float center_x = (float)x + half_w;
            float center_y = (float)y + half_h;

            b2Polygon poly = b2MakeBox(half_w, half_h);
            // set polygon local transform: Box2D polygon shape creation here uses center offset via transform
            // but the C API b2CreatePolygonShape clones polygon geometry; we'll set the body transform instead

            // create shape at the computed position by temporarily translating body
            // create a fixture by creating polygon shape and then setting its transform via b2Shape_SetPolygon later if
            // needed The C API lacks direct per-shape transform at creation, so we will shift the body then attach and
            // shift back.

            // For simplicity create a shape and then set body transform so shape sits in the right place
            // save current body transform
            b2Transform old_xf = b2Body_GetTransform(body);
            b2Vec2 newpos = {center_x, center_y};
            b2Body_SetTransform(body, newpos, b2Rot_identity);

            b2ShapeId shape = b2CreatePolygonShape(body, &shapeDef, &poly);
            if (b2Shape_IsValid(shape))
            {
                ++created_shapes;
            }
            else
            {
                // cleanup and fail
                free(visited);
                if (b2Body_IsValid(body))
                    b2DestroyBody(body);
                return NULL;
            }

            // restore body transform (for subsequent shapes we'll re-position and attach)
            b2Body_SetTransform(body, old_xf.p, old_xf.q);
        }
    }

    free(visited);

    if (created_shapes == 0)
    {
        if (b2Body_IsValid(body))
            b2DestroyBody(body);
        return NULL;
    }

    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)calloc(1, sizeof(PhysicsBodyWrapper));
    if (!bw)
    {
        if (b2Body_IsValid(body))
            b2DestroyBody(body);
        return NULL;
    }
    bw->id = body;
    // shape ids zero-initialized by calloc

    b2Body_SetUserData(body, entity_ptr);
    return (physics_body_t)bw;
}

void physics_destroy_body(physics_world_t w, physics_body_t b)
{
    if (!w || !b)
        return;
    PhysicsWorldWrapper *pw = (PhysicsWorldWrapper *)w;
    if (!b2World_IsValid(pw->id))
        return;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!bw)
        return;
    if (b2Body_IsValid(bw->id))
        b2DestroyBody(bw->id);
    free(bw);
}

void physics_body_get_position(physics_body_t b, b2Vec2 *out_pos)
{
    if (!b || !out_pos)
        return;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!b2Body_IsValid(bw->id))
        return;
    b2Transform xf = b2Body_GetTransform(bw->id);
    *out_pos = xf.p;
}

void physics_body_set_position(physics_body_t b, b2Vec2 pos)
{
    if (!b)
        return;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!b2Body_IsValid(bw->id))
        return;
    b2Transform xf = b2Body_GetTransform(bw->id);
    b2Rot rot = xf.q;
    b2Body_SetTransform(bw->id, pos, rot);
}

float physics_body_get_angle(physics_body_t b)
{
    if (!b)
        return 0.0f;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!b2Body_IsValid(bw->id))
        return 0.0f;
    b2Transform xf = b2Body_GetTransform(bw->id);
    return b2Rot_GetAngle(xf.q);
}

void physics_body_set_angle(physics_body_t b, float a)
{
    if (!b)
        return;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!b2Body_IsValid(bw->id))
        return;
    b2Transform xf = b2Body_GetTransform(bw->id);
    b2Rot r;
    r.s = sinf(a);
    r.c = cosf(a);
    b2Body_SetTransform(bw->id, xf.p, r);
}

void physics_body_get_linear_velocity(physics_body_t b, b2Vec2 *out_vel)
{
    if (!b || !out_vel)
        return;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!b2Body_IsValid(bw->id))
        return;
    *out_vel = b2Body_GetLinearVelocity(bw->id);
}

void physics_body_set_linear_velocity(physics_body_t b, b2Vec2 vel)
{
    if (!b)
        return;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!b2Body_IsValid(bw->id))
        return;
    b2Body_SetLinearVelocity(bw->id, vel);
}

void physics_body_apply_force(physics_body_t b, b2Vec2 force)
{
    if (!b)
        return;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!b2Body_IsValid(bw->id))
        return;
    b2Body_ApplyForceToCenter(bw->id, force, true);
}

physics_joint_t physics_create_weld_joint(physics_world_t w, physics_body_t a, physics_body_t b)
{
    if (!w || !a || !b)
        return NULL;
    PhysicsWorldWrapper *pw = (PhysicsWorldWrapper *)w;
    if (!b2World_IsValid(pw->id))
        return NULL;
    PhysicsBodyWrapper *aw = (PhysicsBodyWrapper *)a;
    PhysicsBodyWrapper *bw = (PhysicsBodyWrapper *)b;
    if (!b2Body_IsValid(aw->id) || !b2Body_IsValid(bw->id))
        return NULL;

    b2WeldJointDef def = b2DefaultWeldJointDef();
    def.bodyIdA = aw->id;
    def.bodyIdB = bw->id;
    def.localAnchorA = b2Vec2_zero;
    def.localAnchorB = b2Vec2_zero;
    def.referenceAngle = 0.0f;

    b2JointId jid = b2CreateWeldJoint(pw->id, &def);
    if (!b2Joint_IsValid(jid))
        return NULL;

    PhysicsJointWrapper *jw = (PhysicsJointWrapper *)malloc(sizeof(PhysicsJointWrapper));
    if (!jw)
    {
        b2DestroyJoint(jid);
        return NULL;
    }
    jw->id = jid;
    return (physics_joint_t)jw;
}

void physics_destroy_joint(physics_world_t w, physics_joint_t j)
{
    if (!w || !j)
        return;
    PhysicsWorldWrapper *pw = (PhysicsWorldWrapper *)w;
    if (!b2World_IsValid(pw->id))
        return;
    PhysicsJointWrapper *jw = (PhysicsJointWrapper *)j;
    if (!jw)
        return;
    if (b2Joint_IsValid(jw->id))
        b2DestroyJoint(jw->id);
    free(jw);
}
