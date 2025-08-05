#include "../include/rendering.h"
#include "../include/block_renderer.h"
#include "../include/flags.h"
#include "../include/vars.h"
#include <excpt.h>

const unsigned int funny_primes[] = {1155501, 6796373, 7883621, 4853063, 8858313,
                                     6307353, 1532671, 6233633, 873473,  685613};
const u8 funny_shifts[] = {9, 7, 5, 3, 1, 2, 4, 6, 8, 10};

static unsigned short tile_rand(const int x, const int y)
{
    int prime = x % 10;
    int antiprime = 9 - (y % 10);
    int midprime = (prime + antiprime) / 2;
    return ((funny_primes[prime] + funny_shifts[antiprime] * funny_primes[midprime]) >> funny_shifts[prime]) & 0x7fff;
}

const float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

u8 render_layer(layer_slice slice)
{
    CHECK(slice.zoom == 0)
    const int local_block_width = (g_block_width * slice.zoom);

    const int width = slice.w / local_block_width;
    const int height = slice.h / local_block_width;

    if (!slice.ref)
        return SUCCESS;

    if (!slice.ref->blocks)
        return SUCCESS;

    const bool is_frozen = FLAG_GET(slice.flags, LAYER_SLICE_FLAG_FROZEN);

    if (is_frozen)
    {
        block_renderer_render_frozen(&slice);
    }
    else
    {
        block_registry *b_reg = slice.ref->registry;
        // Calculate the starting block coordinates based on the slice position
        const int start_block_x = ((slice.x / local_block_width) - 1);
        const int start_block_y = ((slice.y / local_block_width) - 1);

        const int end_block_x = start_block_x + width + 2;
        const int end_block_y = start_block_y + height + 2;

        const int block_x_offset = slice.x % local_block_width; /* offset in pixels for smooth rendering of blocks */
        const int block_y_offset = slice.y % local_block_width;

        const u32 ms_since_start = SDL_GetTicks();
        const float seconds_since_start = ms_since_start / 1000.0f;

        // Normal layer rendering
        GLuint texture = b_reg->atlas_texture_uid;

        u16 dest_x, dest_y;
        dest_x = -block_x_offset - local_block_width * 2;
        // also minus 1 full block back to fill the gap
        block_renderer_begin_batch();

        for (i32 i = start_block_x; i < end_block_x; i++)
        {
            dest_x += local_block_width;
            dest_y = -block_y_offset - local_block_width * 2;

            for (i32 j = start_block_y; j < end_block_y; j++)
            {
                dest_y += local_block_width;
                // calculate y coordinate of block on screen

                if (i > slice.ref->width || j > slice.ref->height)
                    continue;

                // get block
                u64 id = 0;
                if (i >= 0 && j >= 0 && i < slice.ref->width && j < slice.ref->height)
                    id = *BLOCK_ID_PTR(slice.ref, i, j);

                if (id == 0)
                    continue;

                // get block vars
                u8 frame = 0;
                u8 type = 0;
                u8 flip = 0;
                i16 rotation = 0;

                i16 offset_x = 0;
                i16 offset_y = 0;

                block_resources br = b_reg->resources.data[id];

                blob *var = NULL;
                if (block_get_vars(slice.ref, i, j, &var) == SUCCESS && var != NULL && var->length != 0 &&
                    var->ptr != NULL)
                {
                    if (br.type_controller != 0)
                        var_get_u8(*var, br.type_controller, &type);

                    if (br.flip_controller != 0)
                        var_get_u8(*var, br.flip_controller, &flip);

                    if (br.rotation_controller != 0)
                        var_get_i16(*var, br.rotation_controller, &rotation);

                    if (br.anim_controller != 0)
                        var_get_u8(*var, br.anim_controller, &frame);

                    if (br.offset_x_controller != 0)
                        var_get_i16(*var, br.offset_x_controller, &offset_x);

                    if (br.offset_y_controller != 0)
                        var_get_i16(*var, br.offset_y_controller, &offset_y);

                    if (br.interp_takes != 0 && br.interp_timestamp_controller != 0)
                    {
                        u32 ms_started_moving = 0; // tick timestamp when started moving
                        var_get_u32(*var, br.interp_timestamp_controller, &ms_started_moving);

                        // u32 ms_move_takes = 0;
                        // var_get_u32(*var, br.interp_takes_controller, &ms_move_takes);

                        const float pos = (ms_since_start - ms_started_moving) / (float)br.interp_takes;
                        const float clamp_pos = fmax(0.0f, fmin(1.0, pos));

                        offset_x = lerp(offset_x, 0, clamp_pos);
                        offset_y = lerp(offset_y, 0, clamp_pos);

                        // LOG_DEBUG("jumper %d %d : %f %d %d ", ms_started_moving, ms_since_start, clamp_pos, offset_x,
                        //           offset_y);
                    }
                }

                if (br.frames_per_second > 1)
                {
                    int fps = br.frames_per_second;
                    frame = (u8)(seconds_since_start * fps);
                }

                if (FLAG_GET(br.flags, RESOURCE_FLAG_RANDOM_POS))
                {
                    frame = tile_rand(i, j) % br.info.frames;
                }

                if (br.override_frame != 0)
                    frame = br.override_frame;

                block_renderer_create_instance(br.info, dest_x + offset_x, dest_y + offset_y);

                if (frame || type || flip || rotation)
                { // only set properties if they are not default
                    u8 actual_frame = frame % br.info.frames;
                    u8 actual_type = FLAG_GET(br.flags, RESOURCE_FLAG_IGNORE_TYPE) ? (u8)(frame / br.info.frames)
                                                                                   : (type % br.info.types);
                    block_renderer_set_instance_properties(br.info.atlas_offset_x + actual_frame,
                                                           br.info.atlas_offset_y + actual_type, flip, 1.0f, 1.0f,
                                                           (float)rotation * (M_PI / 180.0f));
                }
            }
        }

        block_renderer_end_batch(b_reg->atlas, texture, local_block_width);
    }

    return SUCCESS;
}

u8 client_render(const client_render_rules rules)
{
    for (u32 i = 0; i < rules.slices.length; i++) // look for all unprepared static layers
    {
        layer_slice slice = rules.slices.data[i];

        if (FLAG_GET(slice.flags, LAYER_SLICE_FLAG_FROZEN))
        {
            layer *l = slice.ref;

            u32 width = l->width * g_block_width;
            u32 height = l->height * g_block_width; // Fixed: was using width instead of height

            framebuffer fb = {};

            // Check for existing framebuffer
            if (slice.framebuffer == 0 || slice.framebuffer_texture == 0)
            {
                LOG_DEBUG("Creating framebuffer for static layer %d", i);

                fb = create_framebuffer_object(width, height);
                if (fb.fbo == 0)
                {
                    LOG_ERROR("Failed to create framebuffer for static layer");
                    continue;
                }
            }
            else
            {
                fb = (framebuffer){
                    .fbo = slice.framebuffer,
                    .texture = slice.framebuffer_texture,
                };

                LOG_DEBUG("Using existing framebuffer %d for static layer %d", fb.fbo, i);
            }

            // Bind the framebuffer and clear it before rendering the layer
            block_renderer_bind_custom_framebuffer(fb);

            layer_slice full_slice = slice;
            full_slice.x = 0;
            full_slice.y = 0;
            full_slice.w = width;
            full_slice.h = height;
            full_slice.zoom = 1;

            render_layer(full_slice); // Render this layer to framebuffer

            // Store the framebuffer info in the layer slice
            rules.slices.data[i].framebuffer = fb.fbo;
            rules.slices.data[i].framebuffer_texture = fb.texture;

            FLAG_SET(rules.slices.data[i].flags, LAYER_SLICE_FLAG_FROZEN, 1);
        }
    }

    block_renderer_begin_frame(); // framebuffer ready to soak in pixels

    for (u32 i = 0; i < rules.draw_order.length; i++)
    {
        int layer_id = rules.draw_order.data[i];
        layer_slice slice = rules.slices.data[layer_id];

        render_layer(slice);
    }

    block_renderer_end_frame();

    return SUCCESS;
}