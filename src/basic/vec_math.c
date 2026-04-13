#include "include/vec_math.h"

#include <assert.h>
#include <stdlib.h>

f32 random_float() // [0,1)
{
	return (f32)rand() / ((f32)RAND_MAX + 1.0f); // avoid exactly 1
}

f32 random_float_range(f32 start, f32 end)
{
	return random_float() * (end - start) - start;
}

f32 normal_float(f32 start, f32 end, u8 rolls)
{
	f32 acc = 0.0f;

	for (u32 i = 0; i < rolls; i++)
		acc += random_float();

	f32 mean = (f32)rolls / 2.0f;
	f32 stddev = sqrtf((f32)rolls / 12.0f);

	f32 z_score = (acc - mean) / stddev;

	f32 length = end - start;
	f32 center = start + (length / 2.0f);

	return center + (z_score * (length / 6.0f));
}

void vec2_normalize(vec2 *a)
{
	const f32 inverse_length = 1.0f / VEC2_LEN((*a));

	a->x *= inverse_length;
	a->y *= inverse_length;
}

vec3 vec3_normalize(vec3 a)
{
	const f32 inverse_length = 1.0f / VEC3_LEN(a);

	a.x *= inverse_length;
	a.y *= inverse_length;
	a.z *= inverse_length;

	return a;
}

// quant

quaternion quaternion_from_euler(vec3 a)
{
	const f32 pitch = a.x * (M_PI / 180.0f);
	const f32 yaw = a.y * (M_PI / 180.0f);
	const f32 roll = a.z * (M_PI / 180.0f);

	const f32 cp = cos(pitch * 0.5f);
	const f32 sp = sin(pitch * 0.5f);
	const f32 cy = cos(yaw * 0.5f);
	const f32 sy = sin(yaw * 0.5f);
	const f32 cr = cos(roll * 0.5f);
	const f32 sr = sin(roll * 0.5f);

	quaternion q = {};
	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;

	return q;
}

vec3 quaternion_rotate_vector(quaternion a, vec3 b)
{
	const vec3 q_vec = (vec3){a.x, a.y, a.z};

	vec3 uv = VEC3_CROSS(q_vec, b);
	vec3 uuv = VEC3_CROSS(q_vec, uv);

	f32 mul_uv = (2.0f * a.w);
	uv = VEC3_MUL(uv, mul_uv);
	uuv = VEC3_MUL(uuv, 2.0f);

	return VEC3_ADD(VEC3_ADD(b, uv), uuv);
}

// misc

f32 normalize_yaw(f32 yaw)
{
	while (yaw > 180.0f)
		yaw -= 360.0f;
	while (yaw < -180.0f)
		yaw += 360.0f;
	return yaw;
}

vec3 vector_to_angle(const vec3 forward)
{
	const f32 len = sqrtf(forward.x * forward.x + forward.y * forward.y);
	f32 pitch = RAD_TO_DEG(atan2f(-forward.z, len));
	f32 yaw = RAD_TO_DEG(atan2f(forward.y, forward.x));
	return (vec3){.x = pitch, .y = yaw, .z = 0.f};
}

vec3 calculate_angle(const vec3 src, const vec3 dst)
{
	const vec3 delta = VEC3_SUB(dst, src);
	const f32 horizontal_dist = sqrtf(delta.x * delta.x + delta.y * delta.y);

	vec3 angles;
	angles.x = RAD_TO_DEG(atan2f(-delta.z, horizontal_dist));
	angles.y = RAD_TO_DEG(atan2f(delta.y, delta.x));
	angles.z = 0.0f;

	return angles;
}

f32 calculate_fov(const vec3 view_angles, const vec3 src, const vec3 dst)
{
	const vec3 angle = calculate_angle(src, dst);
	const vec2 delta = (vec2){.x = view_angles.x - angle.x, .y = normalize_yaw(view_angles.y - angle.y)};

	return VEC2_LEN(delta);
}

void normalize_angles(vec3 *angles)
{
	while (angles->y > 180.0f)
		angles->y -= 360.0f;
	while (angles->y < -180.0f)
		angles->y += 360.0f;
	while (angles->x > 89.0f)
		angles->x -= 180.0f;
	while (angles->x < -89.0f)
		angles->x += 180.0f;

	angles->z = 0.0f;
}

#define ANGLE_VEC_SHARED(angles)                                                                                       \
	const f32 pitch = angles.x * (M_PI / 180.0f);                                                                      \
	const f32 yaw = angles.y * (M_PI / 180.0f);                                                                        \
	const f32 roll = angles.z * (M_PI / 180.0f);                                                                       \
	const f32 sin_pitch = sinf(pitch);                                                                                 \
	const f32 cos_pitch = cosf(pitch);                                                                                 \
	const f32 sin_yaw = sinf(yaw);                                                                                     \
	const f32 cos_yaw = cosf(yaw);                                                                                     \
	const f32 sin_roll = sinf(roll);                                                                                   \
	const f32 cos_roll = cosf(roll);

void angle_vectors_f(const vec3 angles, vec3 *forward)
{
	assert(forward);

	const f32 pitch = angles.x * (M_PI / 180.0f);
	const f32 yaw = angles.y * (M_PI / 180.0f);
	const f32 sin_pitch = sinf(pitch);
	const f32 cos_pitch = cosf(pitch);
	const f32 sin_yaw = sinf(yaw);
	const f32 cos_yaw = cosf(yaw);

	forward->x = cos_pitch * cos_yaw;
	forward->y = cos_pitch * sin_yaw;
	forward->z = -sin_pitch;
}

void angle_vectors_r(const vec3 angles, vec3 *right)
{
	assert(right);

	const f32 pitch = angles.x * (M_PI / 180.0f);
	const f32 yaw = angles.y * (M_PI / 180.0f);
	const f32 roll = angles.z * (M_PI / 180.0f);
	const f32 sin_pitch = sinf(pitch);
	const f32 cos_pitch = cosf(pitch);
	const f32 sin_yaw = sinf(yaw);
	const f32 cos_yaw = cosf(yaw);
	const f32 sin_roll = sinf(roll);
	const f32 cos_roll = cosf(roll);

	right->x = -sin_roll * sin_pitch * cos_yaw + cos_roll * sin_yaw;
	right->y = -sin_roll * sin_pitch * sin_yaw - cos_roll * cos_yaw;
	right->z = -sin_roll * cos_pitch;
}

void angle_vectors_u(const vec3 angles, vec3 *up)
{
	assert(up);

	const f32 pitch = angles.x * (M_PI / 180.0f);
	const f32 yaw = angles.y * (M_PI / 180.0f);
	const f32 roll = angles.z * (M_PI / 180.0f);
	const f32 sin_pitch = sinf(pitch);
	const f32 cos_pitch = cosf(pitch);
	const f32 sin_yaw = sinf(yaw);
	const f32 cos_yaw = cosf(yaw);
	const f32 sin_roll = sinf(roll);
	const f32 cos_roll = cosf(roll);

	up->x = cos_roll * sin_pitch * cos_yaw + sin_roll * sin_yaw;
	up->y = cos_roll * sin_pitch * sin_yaw - sin_roll * cos_yaw;
	up->z = cos_roll * cos_pitch;
}

void angle_vectors_all(const vec3 angles, vec3 *forward, vec3 *right, vec3 *up)
{
	assert(forward && right && up);
	ANGLE_VEC_SHARED(angles)

	forward->x = cos_pitch * cos_yaw;
	forward->y = cos_pitch * sin_yaw;
	forward->z = -sin_pitch;

	right->x = -sin_roll * sin_pitch * cos_yaw + cos_roll * sin_yaw;
	right->y = -sin_roll * sin_pitch * sin_yaw - cos_roll * cos_yaw;
	right->z = -sin_roll * cos_pitch;

	up->x = cos_roll * sin_pitch * cos_yaw + sin_roll * sin_yaw;
	up->y = cos_roll * sin_pitch * sin_yaw - sin_roll * cos_yaw;
	up->z = cos_roll * cos_pitch;
}

vec3 rotate_by_quat(const quaternion q, const vec3 v)
{
	const f32 qx = q.x, qy = q.y, qz = q.z, qw = q.w;
	const f32 tx = 2.0f * (qy * v.z - qz * v.y);
	const f32 ty = 2.0f * (qz * v.x - qx * v.z);
	const f32 tz = 2.0f * (qx * v.y - qy * v.x);

	return (vec3){.x = v.x + qw * tx + (qy * tz - qz * ty),
				  .y = v.y + qw * ty + (qz * tx - qx * tz),
				  .z = v.z + qw * tz + (qx * ty - qy * tx)};
}
