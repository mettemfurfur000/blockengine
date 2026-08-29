#ifndef RENDER_ROOM_H
#define RENDER_ROOM_H 1

#include "level.h"
#include "rendering.h"

#include <SDL2/SDL.h>

typedef struct camera
{
	f32 x, y;		   // top-left world pixel of the view (interpolated, displayed)
	f32 old_x, old_y;   // previous position, used for frame interpolation
	f32 target_x, target_y; // desired top-left; camera_update advances x/y toward this
	u32 timestamp_old;  // timestamp of the previous position

	f32 zoom; // zoom multiplier (1 = default block size)

	u16 viewport_w, viewport_h; // visible area in screen pixels

	f32 bounds_w, bounds_h; // room size in world pixels (0 = unbounded). The view is
							// clamped so it never shows outside [0, bounds], i.e. it is
							// not allowed to peek past the room border.

	layer *follow_layer;   // optional follow target (entity or block)
	handle32 follow_entity;

	bool follow_block; // if true, follow a static block coordinate instead of an entity
	u32 follow_bx, follow_by;
} camera;

// Interpolated (currently displayed) top-left world pixel of the view. This must be
// used for screen<->world conversions so picking lines up with what is on screen.
void camera_get_displayed_position(const camera *cam, f32 *out_x, f32 *out_y);

typedef struct room_render_options
{
	bool clear_background;
	f32 background_color[4]; // rgba, 0..1

	bool draw_grid; // debug block grid overlay
	f32 grid_color[4];
} room_render_options;

void camera_init(camera *cam, u16 viewport_w, u16 viewport_h, f32 zoom);
void camera_set_follow(camera *cam, layer *l, handle32 entity);
void camera_set_follow_block(camera *cam, layer *l, u32 block_x, u32 block_y);
void camera_center_on(camera *cam, f32 world_x, f32 world_y);
void camera_update(camera *cam);

void render_room_begin_frame(u16 width, u16 height, const room_render_options *opts);
u8 render_room(room *r, const camera *cam, const room_render_options *opts);
void render_room_end_frame(void);

// Drop-in alternative to client_render() that renders a single room through a camera.
// Uses the module-global options (see render_room_set_options).
u8 client_render_room(room *r, camera *cam);

// Replaces the render options used by client_render_room(). Safe to call from Lua.
void render_room_set_options(const room_render_options *opts);

// Opt-in activation used by the main loop. When both are set, the room/camera
// pipeline is used instead of the legacy client_render(). Leave NULL for default.
extern room *render_room_active;
extern camera *render_room_active_camera;
void render_room_activate(room *r, camera *cam);
void render_room_deactivate(void);

#endif
