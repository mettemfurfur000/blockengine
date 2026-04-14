#ifndef PLATE_MATH_H
#define PLATE_MATH_H

#define _USE_MATH_DEFINES
#include <math.h>

#include "general.h"

typedef struct
{
	f32 x, y;
} vec2;

typedef struct
{
	f32 x, y, z;
} vec3;

typedef struct
{
	f32 x, y, z, w;
} vec4;

typedef vec4 quaternion;

typedef struct
{
	vec3 min, max;
} box3;

// vec2 funcs

#define VEC2_NEG(a)                                                                                                    \
	(vec2)                                                                                                             \
	{                                                                                                                  \
		.x = -a.x, .y = -a.y                                                                                           \
	}

#define VEC2_ADD(a, b)                                                                                                 \
	(vec2)                                                                                                             \
	{                                                                                                                  \
		.x = a.x + b.x, .y = a.y + b.y                                                                                 \
	}

#define VEC2_SUB(a, b)                                                                                                 \
	(vec2)                                                                                                             \
	{                                                                                                                  \
		.x = a.x - b.x, .y = a.y - b.y                                                                                 \
	}

#define VEC2_MUL(a, scalar)                                                                                            \
	(vec2)                                                                                                             \
	{                                                                                                                  \
		.x = a.x * scalar, .y = a.y * scalar                                                                           \
	}

#define VEC2_DIV(a, scalar)                                                                                            \
	(vec2)                                                                                                             \
	{                                                                                                                  \
		.x = a.x / scalar, .y = a.y / scalar                                                                           \
	}

#define VEC2_IS_EQUAL(a, b) (a.x == b.x && a.y == b.y)
#define VEC2_DOT(a, b) (a.x * b.x + a.y * b.y)
#define VEC2_LEN_SQR(a) (a.x * a.x + a.y * a.y)
#define VEC2_LEN(a) sqrt(VEC2_LEN_SQR(a))
#define VEC2_NORMALIZE(a) vec2_normalize(&a)

void vec2_normalize(vec2 *a);

// vec3 funcs

#define VEC3_NEG(a)                                                                                                    \
	(vec3)                                                                                                             \
	{                                                                                                                  \
		.x = -a.x, .y = -a.y, .z = -a.z                                                                                \
	}

#define VEC3_ADD(a, b)                                                                                                 \
	(vec3)                                                                                                             \
	{                                                                                                                  \
		.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z                                                                 \
	}

#define VEC3_SUB(a, b)                                                                                                 \
	(vec3)                                                                                                             \
	{                                                                                                                  \
		.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z                                                                 \
	}

#define VEC3_MUL(a, scalar)                                                                                            \
	(vec3)                                                                                                             \
	{                                                                                                                  \
		.x = a.x * scalar, .y = a.y * scalar, .z = a.z * scalar                                                        \
	}

#define VEC3_DIV(a, scalar)                                                                                            \
	(vec3)                                                                                                             \
	{                                                                                                                  \
		.x = a.x / scalar, .y = a.y / scalar, .z = a.z / scalar                                                        \
	}

#define VEC3_IS_EQUAL(a, b) (a.x == b.x && a.y == b.y && a.z == b.z)
#define VEC3_DOT(a, b) (a.x * b.x + a.y * b.y + a.z * b.z)
#define VEC3_LEN_SQR(a) (a.x * a.x + a.y * a.y + a.z * a.z)
#define VEC3_LEN(a) sqrt(VEC3_LEN_SQR(a))
#define VEC3_NORMALIZE(a) vec3_normalize(a)
#define VEC3_CROSS(a, b)                                                                                               \
	(vec3)                                                                                                             \
	{                                                                                                                  \
		.x = a.y * b.z - a.z * b.y, .y = a.z * b.x - a.x * b.z, .z = a.x * b.y - a.y * b.x                             \
	}

vec3 vec3_normalize(vec3 a);

// quaternion funcs

#define VEC4_FROM_EULER(a, b) quaternion_from_euler(a, b)
#define VEC4_ROTATE(a) quaternion_rotate_vector(a)

quaternion quaternion_from_euler(vec3 a);
vec3 quaternion_rotate_vector(quaternion a, vec3 b);

// matrix 4x4

typedef struct
{
	f32 mat[4][4];
} matrix4x4;

// matrix 3x4

typedef struct
{
	f32 mat[3][4];
} matrix3x4;

// misc

// #define MIN(a, b) (a > b ? b : a)
// #define MAX(a, b) (a < b ? b : a)

#define DEG_TO_RAD(degrees) (degrees * (M_PI / 180.0f))
#define RAD_TO_DEG(radians) (radians * (180.0f / M_PI))

f32 random_float();
f32 random_float_range(f32 start, f32 end);
f32 normal_float(f32 start, f32 end, u8 rolls);

f32 normalize_yaw(f32 yaw);
vec3 vector_to_angle(const vec3 forward);
vec3 calculate_angle(const vec3 src, const vec3 dst);
f32 calculate_fov(const vec3 view_angles, const vec3 src, const vec3 dst);
void normalize_angles(vec3 *angles);

void angle_vectors_f(const vec3 angles, vec3 *forward);
void angle_vectors_r(const vec3 angles, vec3 *right);
void angle_vectors_u(const vec3 angles, vec3 *up);
void angle_vectors_all(const vec3 angles, vec3 *forward, vec3 *right, vec3 *up);
vec3 rotate_by_quat(const quaternion q, const vec3 v);

#endif