#include "include/render_room.h"

#include "include/block_entity.h"
#include "include/block_renderer_v2.h"
#include "include/logging.h"
#include "include/sdl2_basics.h"

#include <box2d/box2d.h>
#include <math.h>

room *render_room_active = NULL;
camera *render_room_active_camera = NULL;

void render_room_activate(room *r, camera *cam)
{
	render_room_active = r;
	render_room_active_camera = cam;
}

void render_room_deactivate(void)
{
	render_room_active = NULL;
	render_room_active_camera = NULL;
}

void camera_init(camera *cam, u16 viewport_w, u16 viewport_h, f32 zoom)
{
	cam->x = 0.0f;
	cam->y = 0.0f;
	cam->old_x = 0.0f;
	cam->old_y = 0.0f;
	cam->timestamp_old = SDL_GetTicks();
	cam->zoom = zoom > 0.0f ? zoom : 1.0f;
	cam->viewport_w = viewport_w;
	cam->viewport_h = viewport_h;
	cam->follow_layer = NULL;
	cam->follow_entity = INVALID_HANDLE;
}

void camera_set_follow(camera *cam, layer *l, handle32 entity)
{
	cam->follow_layer = l;
	cam->follow_entity = entity;
}

void camera_center_on(camera *cam, f32 world_x, f32 world_y)
{
	cam->x = world_x - (cam->viewport_w / cam->zoom) * 0.5f;
	cam->y = world_y - (cam->viewport_h / cam->zoom) * 0.5f;
}

void camera_update(camera *cam)
{
	cam->old_x = cam->x;
	cam->old_y = cam->y;
	cam->timestamp_old = SDL_GetTicks();

	if (cam->follow_layer && cam->follow_entity.active)
	{
		block_entity *e = layer_get_block_entity(cam->follow_layer, cam->follow_entity);
		if (e && b2Body_IsValid(e->b2_body_id))
		{
			b2Vec2 p = b2Body_GetPosition(e->b2_body_id);
			camera_center_on(cam, p.x, p.y);
		}
	}
}

void render_room_begin_frame(u16 width, u16 height, const room_render_options *opts)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	renderer_v2_set_projection_size(width, height);

	if (opts && opts->clear_background)
		glClearColor(opts->background_color[0], opts->background_color[1], opts->background_color[2],
					 opts->background_color[3]);
	else
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT);
}

static void render_room_grid(const camera *cam, const room_render_options *opts)
{
	f32 step = (f32)g_block_width * (f32)cam->zoom;
	if (step < 1.0f)
		return;

	f32 c[4] = {opts->grid_color[0], opts->grid_color[1], opts->grid_color[2], opts->grid_color[3]};

	f32 first_x = cam->x - fmodf(cam->x, step);
	for (f32 gx = first_x; gx < cam->x + cam->viewport_w; gx += step)
		renderer_v2_fill_rect(gx, cam->y, 1.0f, (f32)cam->viewport_h, c);

	f32 first_y = cam->y - fmodf(cam->y, step);
	for (f32 gy = first_y; gy < cam->y + cam->viewport_h; gy += step)
		renderer_v2_fill_rect(cam->x, gy, (f32)cam->viewport_w, 1.0f, c);
}

u8 render_room(room *r, const camera *cam, const room_render_options *opts)
{
	if (!r)
		return SUCCESS;

	f32 zoom = cam->zoom;
	if (zoom < 1.0f)
		zoom = 1.0f;
	if (zoom > 255.0f)
		zoom = 255.0f;
	u8 z = (u8)zoom;

	f32 start_x = cam->x < 0.0f ? 0.0f : cam->x;
	f32 start_y = cam->y < 0.0f ? 0.0f : cam->y;
	f32 old_x = cam->old_x < 0.0f ? 0.0f : cam->old_x;
	f32 old_y = cam->old_y < 0.0f ? 0.0f : cam->old_y;

	for (u32 i = 0; i < r->layers.length; i++)
	{
		layer *l = (layer *)r->layers.data[i];
		if (!l)
			continue;

		layer_slice slice = {0};
		slice.ref = l;
		slice.x = (u32)start_x;
		slice.y = (u32)start_y;
		slice.old_x = (u32)old_x;
		slice.old_y = (u32)old_y;
		slice.timestamp_old = cam->timestamp_old;
		slice.w = cam->viewport_w;
		slice.h = cam->viewport_h;
		slice.zoom = z;
		if (l->flags & LAYER_FLAG_STATIC)
			slice.flags |= LAYER_SLICE_FLAG_FROZEN;

		render_layer(slice);
	}

	if (opts && opts->draw_grid)
		render_room_grid(cam, opts);

	return SUCCESS;
}

void render_room_end_frame(void)
{
	// Post-processing is intentionally skipped here. The legacy
	// renderer_v2_end_frame() FBO pass is currently inverted/broken (see ISSUES.md),
	// so we leave the default framebuffer as-is. A correct post pass can be added later.
}

u8 client_render_room(room *r, camera *cam, room_render_options *opts)
{
	camera_update(cam);
	render_room_begin_frame(cam->viewport_w, cam->viewport_h, opts);
	render_room(r, cam, opts);
	render_room_end_frame();
	return SUCCESS;
}
