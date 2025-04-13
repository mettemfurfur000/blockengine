#include "../include/rendering.h"

const unsigned int funny_primes[] = {1155501, 6796373, 7883621, 4853063, 8858313, 6307353, 1532671, 6233633, 873473, 685613};
const u8 funny_shifts[] = {9, 7, 5, 3, 1, 2, 4, 6, 8, 10};

static unsigned short
tile_rand(const int x, const int y)
{
    int prime = x % 10;
    int antiprime = 9 - (y % 10);
    int midprime = (prime + antiprime) / 2;
    return ((funny_primes[prime] + funny_shifts[antiprime] * funny_primes[midprime]) >> funny_shifts[prime]) &
           0x7fff;
}

u8 render_layer(layer_slice slice)
{
    CHECK(slice.zoom == 0)
    const int local_block_width = (g_block_width * slice.zoom);

    const int width = slice.w / local_block_width; // exact amowunt of lboks to render o nscren
    const int height = slice.h / local_block_width;

    if (!slice.ref)
        return SUCCESS;

    // if (slice.ref->bytes_per_block == 1)
    // {
    //     bprintf(slice.ref, 0, 1, 32, "%d    %d    ", slice.x, slice.y);
    //     bprintf(slice.ref, 0, 2, 32, "%d    %d    ", slice.w, slice.h);
    //     // bprintf(slice.ref, 0, 4, 32, "%d    %d    %d    %d    ", start_block_x / chunk_width, start_block_y / chunk_width, 1 + end_block_x / chunk_width, 1 + end_block_y / chunk_width);
    // }

    block_registry *b_reg = slice.ref->registry;

    // LOG_DEBUG("finding registry %p", b_reg);

    const int start_block_x = ((slice.x / local_block_width) - 1);
    const int start_block_y = ((slice.y / local_block_width) - 1);

    const int end_block_x = start_block_x + width + 2;
    const int end_block_y = start_block_y + height + 2;

    const int block_x_offset = slice.x % local_block_width; /* offset in pixels for smooth rendering of blocks */
    const int block_y_offset = slice.y % local_block_width;

    float dest_x, dest_y;
    dest_x = -block_x_offset - local_block_width * 2; // also minus 1 full block back to fill the gap

    // if (layer_index == 0)
    // {
    // 	bprintf(w, b_reg, 0, 0, 2, 32, "start coords: %d    %d    ", -block_x_offset - local_block_width, -block_y_offset - local_block_width);
    // }

    texture *texture;

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
                block_get_id(slice.ref, i, j, &id);
            // check if block is not void
            if (id == 0)
                continue;
            // check if block could exist in registry
            if (id >= b_reg->resources.length)
                continue;

            if (b_reg->resources.length <= 0)
            {
                LOG_DEBUG("weird\n");
                continue;
            }

            block_resources br = b_reg->resources.data[id];

            // get texture
            texture = &br.block_texture;
            if (!texture)
                continue;

            // get block vars

            u8 frame = 0;
            u8 type = 0;
            u8 flip = 0;
            u16 rotation = 0;

            blob *var = NULL;
            if (block_get_vars(slice.ref, i, j, &var) == SUCCESS && var != NULL && var->length != 0 && var->ptr != NULL)
            {
                if (br.type_controller != 0)
                    var_get_u8(*var, br.type_controller, &type);

                if (br.flip_controller != 0)
                    var_get_u8(*var, br.flip_controller, &flip);

                if (br.rotation_controller != 0)
                    var_get_u16(*var, br.rotation_controller, &rotation);

                if (br.anim_controller != 0)
                    var_get_u8(*var, br.anim_controller, &frame);
            }

            if (br.frames_per_second > 1)
            {
                float seconds_since_start = SDL_GetTicks() / 1000.0f;
                int fps = br.frames_per_second;
                frame = (u8)(seconds_since_start * fps);
            }

            if (FLAG_GET(br.flags, B_RES_FLAG_RANDOM_POS))
            {
                frame = tile_rand(i, j);
            }

            if (br.override_frame != 0)
                frame = br.override_frame;

            block_render(texture, dest_x, dest_y, frame, type, FLAG_GET(br.flags, B_RES_FLAG_IGNORE_TYPE), local_block_width, flip, rotation);
        }
    }

    return SUCCESS;
}

u8 client_render(const client_render_rules rules)
{
    for (u32 i = 0; i < rules.draw_order.length; i++)
    {
        int layer_id = rules.draw_order.data[i];
        render_layer(rules.slices.data[layer_id]);
    }

    return SUCCESS;
}