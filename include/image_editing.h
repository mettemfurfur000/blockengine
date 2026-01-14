#ifndef IMAGE_EDITING_H
#define IMAGE_EDITING_H

#include "general.h"

typedef struct image
{
    union
    {
        u8 *data;
        u8 *pixels;
    };

    i32 width;
    i32 height;
} image;

#define CHANNELS 4
#define ACCESS_CHANNEL(img, x, y, c) (img->data + (((y) * img->width + (x)) * CHANNELS + (c)))

// Basic image manipulation functions
image *image_create(u16 width, u16 height);
void image_free(image *img);
image *image_copy(const image *src);

// Image loading and saving
image *image_load(const char *filename);
void image_save(const image *img, const char *filename);

// Color manipulation
void image_adjust_brightness(image *img, float factor);
void image_apply_color(image *img, u8 color[4]);
void image_gamma_correction(image *img, float gamma);

// Geometric transformations
image *image_crop(const image *src, u16 x, u16 y, u16 width, u16 height);
image *image_rotate(const image *src, i8 clockwise_rotations);
image *image_flip_horizontal(const image *src);
image *image_flip_vertical(const image *src);

// Compositing
// void blend_images(image *dst, const image *src, float ratio);
void image_overlay(image *dst, const image *src, u16 x, u16 y);

// Utility functions
void image_clear(image *img);
void image_fill_color(image *img, u8 color[4]);
void image_get_avg_color(image *img, u8 color_out[4]);

#endif