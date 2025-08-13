#include "../include/atlas_builder.h"
#include "../include/flags.h"
#include "../include/image_editing.h"
#include "../include/opengl_stuff.h"
#include <math.h>

int __img_cmp(const void *a, const void *b)
{
    block_resources __a = {};
    block_resources __b = {};

    if (__a.img == NULL)
        return 1;
    if (__b.img == NULL)
        return -1;

    u32 vol_a = __a.img->width * __a.img->height;
    u32 vol_b = __b.img->width * __b.img->height;

    return (vol_a <= vol_b) ? -1 : 1;
}

// smollest go to de end
void sort_by_volume(block_registry *reg)
{
    qsort(reg->resources.data, reg->resources.length, sizeof(*reg->resources.data), __img_cmp);
}

bool will_fit(u8 *obuf, u32 obuf_w, u32 obuf_h, u32 x, u32 y, u32 w, u32 h)
{
    if (x + w > obuf_w || y + h > obuf_h)
        return false;
    for (u32 j = 0; j < h; j++)
        for (u32 i = 0; i < w; i++)
        {
            u32 offset = y * obuf_w + x + j * obuf_w + i;
            if (obuf[offset])
                return false;
        }

    return true;
}

void mark_taken(u8 *obuf, u32 obuf_w, u32 obuf_h, u32 x, u32 y, u32 w, u32 h)
{
    for (u32 j = 0; j < h; j++)
        for (u32 i = 0; i < w; i++)
        {
            u32 offset = y * obuf_w + x + j * obuf_w + i;
            obuf[offset] = 0xff;
        }
}

void build_atlas(block_registry *reg)
{
    u32 total_height = 0;
    u32 total_width = 0;

    u64 total_pixels = 0;

    const u32 len = reg->resources.length;

    for (u32 i = 0; i < len; i++)
    {
        block_resources *r = &reg->resources.data[i];

        if (r->img == NULL)
        {
            LOG_DEBUG("no texture for block %d", i);
            continue;
        }

        if (r->img->width == 0 || r->img->height == 0 || r->id == 0 || FLAG_GET(r->flags, RESOURCE_FLAG_IS_FILLER) ||
            FLAG_GET(r->flags, RESOURCE_FLAG_RANGED))
            continue;

        total_height += r->img->height;
        total_width += r->img->width;
        total_pixels += r->img->height * r->img->width;

        // LOG_DEBUG("appending %dx%d from %s", r->img->height, r->img->width, r->texture_filename);
    }

    u32 min_side = sqrt(total_pixels);

    // guessed size in pixels

    u32 guess_w = pow(2, (u32)log2((min_side + total_width) / 4.0f));
    u32 guess_h = pow(2, (u32)log2((min_side + total_height) / 4.0f));
    // u32 guess_w = pow(2, (u32)log2(min_side + (total_width)));
    // u32 guess_h = pow(2, (u32)log2(min_side + (total_height)));
build_again:;
    u32 obuf_w = guess_w / g_block_width;
    u32 obuf_h = guess_h / g_block_width;

    u8 *obuf = calloc(obuf_w * obuf_h, 1);

    LOG_DEBUG("atlas generator guessed %dx%d", guess_w, guess_h);

    image *atlas = create_image(guess_w, guess_h);

    // sort by width
    sort_by_volume(reg);

    u32 index = 0;

    for (u32 j = 0; j < obuf_h; j++)
        for (u32 i = 0; i < obuf_w; i++)
        {
            if (index == reg->resources.length)
                continue;

            block_resources *r = &reg->resources.data[index];

            image *img = r->img;

            if (!img)
            {
                i--;
                index++;
                continue;
            }

            if (img->width == 0 || img->height == 0 || r->id == 0 || FLAG_GET(r->flags, RESOURCE_FLAG_IS_FILLER) ||
                FLAG_GET(r->flags, RESOURCE_FLAG_RANGED))
            {
                index++;
                i--;
                continue;
            }

            u32 tex_w = img->width / g_block_width;
            u32 tex_h = img->height / g_block_width;

            // if not taken and will fit
            if (obuf[j * obuf_w + i] == 0 && will_fit(obuf, obuf_w, obuf_h, i, j, tex_w, tex_h))
            {
                // LOG_DEBUG("atlas gen successful fit: %s at %d,%d : %dx%d", r->texture_filename, i, j, tex_w, tex_h);
                // mark as taken and put it on da atlas
                mark_taken(obuf, obuf_w, obuf_h, i, j, tex_w, tex_h);
                overlay_image(atlas, img, i * g_block_width, j * g_block_width);

                // also write our offset to de registry, in pixels
                r->info.atlas_offset_x = i;
                r->info.atlas_offset_y = j;

                // since we skipped all the ranged entries we need to update
                // their atlas offsets too

                for (u32 b = 0; b < reg->resources.length; b++)
                {
                    block_resources *blk = &reg->resources.data[b];

                    if (FLAG_GET(blk->flags, RESOURCE_FLAG_RANGED) &&
                        (strcmp(blk->texture_filename, r->texture_filename) == 0))
                    {
                        blk->info = r->info;
                    }
                }

                index++; // iterate another texture
            }
        }

    free(obuf);

    if (index != reg->resources.length)
    {
        LOG_ERROR("Wrong atlas guess");
    
        if (guess_h < guess_w)
            guess_h *= 2;
        else
            guess_w *= 2;
    
        free_image(atlas);
        goto build_again;
    }

    save_image(atlas, "atlas_latest_debug.png"); // REMOVE ME MAYBE
    sort_by_id(&reg->resources);

    reg->atlas_texture_uid = gl_bind_texture(atlas);

    // free_image(atlas);
    reg->atlas = atlas;
}
