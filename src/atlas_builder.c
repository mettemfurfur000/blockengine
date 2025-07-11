#include "../include/atlas_builder.h"
#include "../include/flags.h"
#include "../include/image_editing.h"
#include <math.h>

int __img_cmp(const void *a, const void *b)
{
    block_resources __a = {};
    block_resources __b = {};

    if (__a.img == NULL || __b.img == NULL)
        return 0;

    u32 vol_a = sqrt(__a.img->width * __a.img->width +
                     __a.img->height * __a.img->height);
    u32 vol_b = sqrt(__b.img->width * __b.img->width +
                     __b.img->height * __b.img->height);

    return (vol_a > vol_b) - (vol_a < vol_b);
}

// smollest go to de end
void sort_by_volume(block_registry *reg)
{
    qsort(reg->resources.data, reg->resources.length,
          sizeof(*reg->resources.data), __img_cmp);
}

bool will_fit(u8 *obuf, u32 obuf_w, u32 obuf_h, u32 x, u32 y, u32 w, u32 h)
{
    if (x + w > obuf_w || y + h > obuf_h)
        return false;
    for (u32 j = 0; j < h; j++)
        for (u32 i = 0; i < w; i++)
        {
            // first (or might be only) slot of a texture
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
            // first (or might be only) slot of a texture
            u32 offset = y * obuf_w + x + j * obuf_w + i;
            obuf[offset] = 0xff; // set it to the most positive value possible
                                 // to explicity mark it as TAKEN
        }
}

void build_atlas(block_registry *reg)
{
    // seems like the only way to calculate our block limit without being too
    // freaky with macro's u32 block_limit = pow(2, sizeof(reg->atlas->height) *
    // 8) / 16;

    u32 total_height = 0;
    u32 total_width = 0;

    u64 total_pixels = 0;

    const u32 len = reg->resources.length;

    for (u32 i = 0; i < len; i++)
    {
        block_resources r = {};
        r = reg->resources.data[i];

        if (r.img == NULL)
        {
            LOG_DEBUG("no texture for block %d", i);
            continue;
        }

        total_height += r.img->height;
        total_width += r.img->width;
        total_pixels += r.img->height * r.img->width;
    }

    u32 min_side = sqrt(total_pixels);

    // guessed size in pixels

    u32 guess_w = pow(2, (u32)log2(min_side + sqrt(total_width)));
    u32 guess_h = pow(2, (u32)log2(min_side + sqrt(total_height)));
    // obstruction buffer
    // records which texture "slots" were taken and which ar still free
    u32 obuf_w = guess_w / g_block_width;
    u32 obuf_h = guess_h / g_block_width;

    u8 *obuf = calloc(obuf_w * obuf_h, 1);

    LOG_DEBUG("atlas generator guessed %dx%d", guess_w, guess_h);

    image *atlas = create_image(guess_w, guess_h);

    // sort by width
    sort_by_volume(reg);
    // biggest should be first
    // start putting it on de atlas
    u32 index = 1;

    // i and j point at texture slots, not pixels
    for (u32 j = 0; j < obuf_h; j++)
        for (u32 i = 0; i < obuf_w; i++)
        {
            block_resources *reg_entry = reg->resources.data + index;

            if (index == reg->resources.length)
                continue;
            image *img = reg_entry->img;

            if (!img)
            {
                index++;
                continue;
            }

            // skip completely empty textures, fillers, void, and ranged
            // textures

            if (img->width == 0 || img->height == 0 || reg_entry->id == 0 ||
                FLAG_GET(reg_entry->flags, RESOURCE_FLAG_IS_FILLER) ||
                FLAG_GET(reg_entry->flags, RESOURCE_FLAG_RANGED))
            {
                index++;
                i--;
                continue;
            }

            u32 tex_w = img->width / g_block_width;
            u32 tex_h = img->height / g_block_width;

            // if not taken and will fit
            if (obuf[j * obuf_w + i] == 0 &&
                will_fit(obuf, obuf_w, obuf_h, i, j, tex_w, tex_h))
            {
                LOG_DEBUG("at_gen: %d fits at %d,%d : %dx%d",
                          reg->resources.data[index].id, i, j, tex_w, tex_h);
                // mark as taken and put it on da atlas
                mark_taken(obuf, obuf_w, obuf_h, i, j, tex_w, tex_h);
                overlay_image(atlas, img, i * g_block_width, j * g_block_width);

                // also write our offset to de registry, in pixels
                reg->resources.data[index].info.atlas_offset_x =
                    i * g_block_width;
                reg->resources.data[index].info.atlas_offset_y =
                    j * g_block_width;

                // also free up that space
                // SAFE_FREE(img->pixels);
                // ignore for now

                index++; // iterate another texture
            }
        }

    // all this dividing and multiplication can be kinda expensive but we ar
    // only doing it once! right?

    // all deez registry operations takin so looong, i wish we coud run it once
    // and burn it in some kind of binary file dat compiler coud optimize for us
    // maybe?
    // TODO: make a test program that will read de registry and generate a
    // source file that will replicate all of it but in C code instead

    // TODO: we might also save de atlas somewhere for future starts, mayber
    save_image(atlas, "atlas_latest.png");

    // make it an actual opengl texture, bind it to all blocks and release the
    // image

    // also sort the registry back, you moron!

    sort_by_id(&reg->resources);

    // TODO:
    // make it a texture HERE
    //
    // reg->atlas = texture_bind_gl(atlas);

    // free_image(atlas);
}
