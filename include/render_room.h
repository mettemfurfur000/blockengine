#ifndef RENDER_ROOM_H
#define RENDER_ROOM_H 1

#include "level.h"
#include "rendering.h"

#include <SDL2/SDL.h>

typedef struct camera
{
	f32 x, y;		   // top-left world pixel of the view
	f32 old_x, old_y;   // previous position, used for frame interpolation
	u32 timestamp_old;  // timestamp of the previous position

	f32 zoom; // zoom multiplier (1 = default block size)

	u16 viewport_w, viewport_h; // visible area in screen pixels

	layer *follow_layer;   // optional entity follow target
	handle32 follow_entity;
} camera;

typedef struct room_render_options
{
	bool clear_background;
	f32 background_color[4]; // rgba, 0..1

	bool draw_grid; // debug block grid overlay
	f32 grid_color[4];
} room_render_options;

void camera_init(camera *cam, u16 viewport_w, u16 viewport_h, f32 zoom);
void camera_set_follow(camera *cam, layer *l, handle32 entity);
void camera_center_on(camera *cam, f32 world_x, f32 world_y);
void camera_update(camera *cam);

void render_room_begin_frame(u16 width, u16 height, const room_render_options *opts);
u8 render_room(room *r, const camera *cam, const room_render_options *opts);
void render_room_end_frame(void);

// Drop-in alternative to client_render() that renders a single room through a camera.
u8 client_render_room(room *r, camera *cam, room_render_options *opts);

// Opt-in activation used by the main loop. When both are set, the room/camera
// pipeline is used instead of the legacy client_render(). Leave NULL for default.
extern room *render_room_active;
extern camera *render_room_active_camera;
void render_room_activate(room *r, camera *cam);
void render_room_deactivate(void);

#endif
