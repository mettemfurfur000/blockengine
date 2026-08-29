#include "include/render_room.h"

#include "include/block_entity.h"
#include "include/block_renderer_v2.h"
#include "include/handle.h"
#include "include/logging.h"
#include "include/sdl2_basics.h"
#include "include/config.h"

#include <box2d/box2d.h>
#include <math.h>

room *render_room_active = NULL;
camera *render_room_active_camera = NULL;

void render_room_activate(room *r, camera *cam)
{
	render_room_active = r;
	render_room_active_camera = cam;

	if (cam && r)
	{
		// Clamp the view to the room so the camera never peeks into negative space or
		// past the far block edge.
		cam->bounds_w = (f32)r->width * g_block_width;
		cam->bounds_h = (f32)r->height * g_block_width;
	}
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
	cam->target_x = 0.0f;
	cam->target_y = 0.0f;
	cam->timestamp_old = SDL_GetTicks();
	cam->zoom = zoom > 0.0f ? zoom : 1.0f;
	cam->viewport_w = viewport_w;
	cam->viewport_h = viewport_h;
	cam->follow_layer = NULL;
	cam->follow_entity = INVALID_HANDLE;
	cam->follow_block = false;
	cam->follow_bx = 0;
	cam->follow_by = 0;
	cam->bounds_w = 0.0f;
	cam->bounds_h = 0.0f;
}

void camera_set_follow(camera *cam, layer *l, handle32 entity)
{
	cam->follow_layer = l;
	cam->follow_entity = entity;
	cam->follow_block = false;
}

void camera_set_follow_block(camera *cam, layer *l, u32 block_x, u32 block_y)
{
	cam->follow_layer = l;
	cam->follow_block = true;
	cam->follow_bx = block_x;
	cam->follow_by = block_y;
}

void camera_center_on(camera *cam, f32 world_x, f32 world_y)
{
	// The renderer maps a world pixel W to screen (W * zoom - slice.x), so to keep
	// world_x at the centre of the viewport (screen = viewport_w/2) the slice top-left
	// must be (world_x * zoom - viewport_w/2). Note the world coordinate is scaled by
	// zoom, which is what keeps the camera locked on target at any zoom level.
	cam->target_x = world_x * cam->zoom - (cam->viewport_w * 0.5f);
	cam->target_y = world_y * cam->zoom - (cam->viewport_h * 0.5f);
}

void camera_update(camera *cam)
{
	if (cam->follow_layer)
	{
		if (cam->follow_block)
		{
			cam->target_x = (((f32)cam->follow_bx + 0.5f) * g_block_width) * cam->zoom - (cam->viewport_w * 0.5f);
			cam->target_y = (((f32)cam->follow_by + 0.5f) * g_block_width) * cam->zoom - (cam->viewport_h * 0.5f);
		}
		else if (handle_is_valid(cam->follow_layer->block_entity_pool, cam->follow_entity))
		{
			block_entity *e = layer_get_block_entity(cam->follow_layer, cam->follow_entity);
			if (e && b2Body_IsValid(e->b2_body_id))
			{
				b2Vec2 p = b2Body_GetPosition(e->b2_body_id);
				cam->target_x = p.x * cam->zoom - (cam->viewport_w * 0.5f);
				cam->target_y = p.y * cam->zoom - (cam->viewport_h * 0.5f);
			}
		}
	}

	// Clamp the desired top-left to the room. The slice top-left lives in the same
	// scaled space the renderer uses (screen = world*zoom - slice.x), so the room of
	// world width bounds_w maps to a slice range [0, bounds_w*zoom - viewport_w].
	// This keeps the camera from peeking over the room border (negative coords / past
	// the far block edge) and stops a zoom-out from flinging the view far away.
	f32 view_w = cam->viewport_w;
	f32 view_h = cam->viewport_h;

	if (cam->bounds_w > 0.0f)
	{
		f32 max_x = cam->bounds_w * cam->zoom - view_w;
		if (max_x < 0.0f)
			max_x = 0.0f;
		if (cam->target_x < 0.0f)
			cam->target_x = 0.0f;
		else if (cam->target_x > max_x)
			cam->target_x = max_x;
	}
	if (cam->bounds_h > 0.0f)
	{
		f32 max_y = cam->bounds_h * cam->zoom - view_h;
		if (max_y < 0.0f)
			max_y = 0.0f;
		if (cam->target_y < 0.0f)
			cam->target_y = 0.0f;
		else if (cam->target_y > max_y)
			cam->target_y = max_y;
	}

	// Snap the displayed position straight to the target. The followed block is a
	// grid block that also moves discretely, so locking the camera exactly to the
	// target keeps them aligned at any zoom and the camera keeps up instantly when
	// the block leaves a clamped corner (no tick of lag behind). Keeping old == x
	// makes render_room's slice lerp a no-op.
	cam->x = cam->target_x;
	cam->y = cam->target_y;
	cam->old_x = cam->target_x;
	cam->old_y = cam->target_y;
	cam->timestamp_old = SDL_GetTicks();
}

void camera_get_displayed_position(const camera *cam, f32 *out_x, f32 *out_y)
{
	f32 f = (f32)(SDL_GetTicks() - cam->timestamp_old) / (1000.0f / (f32)TPS);
	if (f < 0.0f)
		f = 0.0f;
	if (f > 1.0f)
		f = 1.0f;
	*out_x = cam->old_x + (cam->x - cam->old_x) * f;
	*out_y = cam->old_y + (cam->y - cam->old_y) * f;
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

	// World grid line at block b is at world pixel b*g_block_width, i.e. screen
	// (b*g_block_width*zoom - cam->x). So the lines are at -cam->x + k*step.
	f32 first_x = -fmodf(cam->x, step);
	for (f32 gx = first_x; gx < cam->viewport_w; gx += step)
		renderer_v2_fill_rect(gx, 0.0f, 1.0f, (f32)cam->viewport_h, c);

	f32 first_y = -fmodf(cam->y, step);
	for (f32 gy = first_y; gy < cam->viewport_h; gy += step)
		renderer_v2_fill_rect(0.0f, gy, (f32)cam->viewport_w, 1.0f, c);
}

u8 render_room(room *r, const camera *cam, const room_render_options *opts)
{
	if (!r)
		return SUCCESS;

	f32 zoom = cam->zoom;
	if (zoom < 1.0f)
		zoom = 1.0f;
	if (zoom > 64.0f)
		zoom = 64.0f;

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
		slice.timestamp_old = cam->timestamp_old;
		slice.w = cam->viewport_w;
		slice.h = cam->viewport_h;
		slice.zoom = zoom;

		if (l->flags & LAYER_FLAG_UI)
		{
			// UI layers are anchored to the screen and do not scroll with the camera.
			slice.x = 0;
			slice.y = 0;
			slice.old_x = 0;
			slice.old_y = 0;
		}
		else
		{
			slice.x = (u32)start_x;
			slice.y = (u32)start_y;
			slice.old_x = (u32)old_x;
			slice.old_y = (u32)old_y;
		}

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

static room_render_options g_render_room_opts = {
	.clear_background = true,
	.background_color = {0.08f, 0.08f, 0.12f, 1.0f},
	.draw_grid = false,
	.grid_color = {0.30f, 0.30f, 0.30f, 0.50f},
};

void render_room_set_options(const room_render_options *opts)
{
	if (opts == NULL)
		return;
	g_render_room_opts = *opts;
}

u8 client_render_room(room *r, camera *cam)
{
	camera_update(cam);
	render_room_begin_frame(cam->viewport_w, cam->viewport_h, &g_render_room_opts);
	render_room(r, cam, &g_render_room_opts);
	render_room_end_frame();
	return SUCCESS;
}
