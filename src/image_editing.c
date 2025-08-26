#include "../include/image_editing.h"

#include "../include/general.h"
#include "../include/sdl2_basics.h"
#include "../stb/stb_image.h"
#include "../stb/stb_image_write.h"

image *create_image(u16 width, u16 height)
{
    LOG_DEBUG("create_image: %dx%d", width, height);

    image *img = (image *)malloc(sizeof(image));

    assert(img != NULL);

    img->width = width;
    img->height = height;

    img->pixels = calloc(height * width, CHANNELS);
    assert(img->pixels != NULL);

    return img;
}

void free_image(image *img)
{
    LOG_DEBUG("free_image: %dx%d", img->width, img->height);

    SAFE_FREE(img->pixels);
    SAFE_FREE(img);
}

image *copy_image(const image *src)
{
    LOG_DEBUG("copy_image: %dx%d", src->width, src->height);

    image *img = create_image(src->width, src->height);
    memcpy(img->pixels, src->pixels, src->width * src->height * CHANNELS);
    return img;
}

// util
#define UPD(a, b, op) (a) = op(a, b)

#define CHANNEL_MULTIPLY(a, b) (u8)((a / 255.0f) * (b / 255.0f))
#define CHANNEL_BLEND(a, b) (u8)(255 - ((255 - a) * (255 - b) / 255.0f))
#define CHANNEL_OVERLAY(a, b) a < 128 ? 2 * a *b / 255.0f : 255 - 2 * (255 - a) * (255 - b) / 255.0f

#define CHANNEL_GAMMA_CORRECTION(a, gamma) (u8)(pow(a / 255.0f, 1.0f / gamma) * 255.0f)

// file system

image *load_image(const char *filename)
{
    i32 width, height, channels;
    u8 *image_data;

    if (!(image_data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha)))
    {
        LOG_ERROR("texture_load Error: stbi_load failed, no such file: %s", filename);
        return NULL;
    }

    // LOG_DEBUG("load_image: %s %dx%d", filename, width, height);

    // image *img = create_image(width, height, channels);

    image *img = (image *)malloc(sizeof(image));

    assert(img != NULL);

    img->width = width;
    img->height = height;

    img->data = image_data;

    return img;
}

void save_image(const image *img, const char *filename)
{
    LOG_DEBUG("save_image: %s", filename);

    stbi_write_png(filename, img->width, img->height, CHANNELS, img->pixels, img->width * CHANNELS);
}

// funny part

void adjust_brightness(image *img, float factor)
{
    for (u32 j = 0; j < img->height; j++)
        for (u32 i = 0; i < img->width; i++)
            for (u32 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(img, i, j, c) = KEEPINLIMITS(*ACCESS_CHANNEL(img, i, j, c) * factor, 0x00, 0xff);
}

void apply_color(image *img, u8 color[4])
{
    LOG_DEBUG("apply_color: %d %d %d %d", color[0], color[1], color[2], color[3]);

    for (u32 j = 0; j < img->height; j++)
        for (u32 i = 0; i < img->width; i++)
            for (u32 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(img, i, j, c) = CHANNEL_BLEND(color[c], *ACCESS_CHANNEL(img, i, j, c));
}

void gamma_correction(image *img, float gamma)
{
    for (u32 j = 0; j < img->height; j++)
        for (u32 i = 0; i < img->width; i++)
            for (u32 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(img, i, j, c) = CHANNEL_GAMMA_CORRECTION(*ACCESS_CHANNEL(img, i, j, c), gamma);
}

image *crop_image(const image *src, u16 x, u16 y, u16 width, u16 height)
{
    if (x >= src->width || y >= src->height)
    {
        LOG_ERROR("crop_image Error: out of bounds");
        return NULL;
    }

    if (width == 0 || height == 0)
    {
        LOG_ERROR("crop_image Error: width or height is 0");
        return NULL;
    }

    u16 end_x = x + width;
    u16 end_y = y + height;

    if (end_x > src->width || end_y > src->height)
    {
        LOG_ERROR("crop_image Error: out of bounds");
        return NULL;
    }

    image *img = create_image(width, height);

    for (u32 j = y; j < end_y; j++)
        for (u32 i = x; i < end_x; i++)
            for (u32 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(img, i - x, j - y, c) = *ACCESS_CHANNEL(src, i, j, c);

    return img;
}

image *rotate_image(const image *src, i8 clockwise_rotations)
{
    if (clockwise_rotations == 0)
    {
        return copy_image(src);
    }

    clockwise_rotations = clockwise_rotations % 4;

    if (clockwise_rotations < 0)
        clockwise_rotations = 4 + clockwise_rotations;

    // swap width and height if rotation is not 180 degrees
    image *img = NULL;
    if (clockwise_rotations % 2 == 0)
        img = create_image(src->width, src->height);
    else
        img = create_image(src->height, src->width);

    for (u32 j = 0; j < src->height; j++)
        for (u32 i = 0; i < src->width; i++)
        {
            u32 dest_x, dest_y;
            switch (clockwise_rotations)
            {
            case 0:
                dest_x = i;
                dest_y = src->height - j - 1;
            case 1: // 90 degrees
                dest_x = src->height - j - 1;
                dest_y = i;
                break;
            case 2: // 180 degrees
                dest_x = src->width - i - 1;
                dest_y = src->height - j - 1;
                break;
            case 3: // 270 degrees
                dest_x = j;
                dest_y = src->width - i - 1;
                break;
            default:
                LOG_ERROR("rotate_image Error: invalid rotation");
                return NULL;
            }

            for (u8 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(img, dest_x, dest_y, c) = *ACCESS_CHANNEL(src, i, j, c);
        }

    return img;
}

image *flip_image_horizontal(const image *src)
{
    image *img = create_image(src->width, src->height);

    for (u32 j = 0; j < src->height; j++)
        for (u32 i = 0; i < src->width; i++)
            for (u8 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(img, i, j, c) = *ACCESS_CHANNEL(src, src->width - i - 1, j, c);

    return img;
}

image *flip_image_vertical(const image *src)
{
    image *img = create_image(src->width, src->height);

    for (u32 j = 0; j < src->height; j++)
        for (u32 i = 0; i < src->width; i++)
            for (u8 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(img, i, j, c) = *ACCESS_CHANNEL(src, i, src->height - j - 1, c);

    return img;
}

// more fun stuff

void overlay_image(image *dst, const image *src, u16 x, u16 y)
{
    if (x >= dst->width || y >= dst->height)
    {
        LOG_ERROR("overlay_image Error: out of bounds");
        return;
    }

    if (src->width == 0 || src->height == 0)
    {
        LOG_ERROR("overlay_image Error: source image is empty");
        return;
    }

    u16 end_x = x + src->width;
    u16 end_y = y + src->height;

    if (end_x > dst->width || end_y > dst->height)
    {
        // instead of erroring we just clip the image to fit in the destination area
        end_x = dst->width;
        end_y = dst->height;
    }

    for (u32 j = y; j < end_y; j++)
        for (u32 i = x; i < end_x; i++)
            for (u8 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(dst, i, j, c) =
                    CHANNEL_BLEND(*ACCESS_CHANNEL(src, i - x, j - y, c), *ACCESS_CHANNEL(dst, i, j, c));
}

// Utility functions
void clear_image(image *img)
{
    if (img == NULL)
    {
        LOG_ERROR("clear_image Error: image is NULL");
        return;
    }

    memset(img->data, 0, img->width * img->height * CHANNELS);
}

void fill_color(image *img, u8 color[4])
{
    if (img == NULL)
    {
        LOG_ERROR("fill_color Error: image is NULL");
        return;
    }

    for (u32 j = 0; j < img->height; j++)
        for (u32 i = 0; i < img->width; i++)
            for (u8 c = 0; c < CHANNELS; c++)
                *ACCESS_CHANNEL(img, i, j, c) = color[c];
}

void get_avg_color_noalpha(image *img, u8 color_out[4])
{
    if (img == NULL)
    {
        LOG_ERROR("fill_color Error: image is NULL");
        return;
    }

    const u32 colors = img->height * img->width;

    u32 color[4] = {};

    for (u32 j = 0; j < img->height; j++)
        for (u32 i = 0; i < img->width; i++)
            for (u8 c = 0; c < CHANNELS; c++)
                color[c] += *ACCESS_CHANNEL(img, i, j, c);

    for (u8 c = 0; c < CHANNELS; c++)
        color_out[c] = color[c] / colors;
}
