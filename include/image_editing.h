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

    // char* filename;

    i32 width;
    i32 height;
} image;

// expected to be RGBA, i dunno bout any other formats mate!

#define CHANNELS 4
#define ACCESS_CHANNEL(img, x, y, c) (img->data + (((y) * img->width + (x)) * CHANNELS + (c)))

// Basic image manipulation functions
image *create_image(u16 width, u16 height);
void free_image(image *img);
image *copy_image(const image *src);

// Image loading and saving
image *load_image(const char *filename);
void save_image(const image *img, const char *filename);

// Color manipulation
void adjust_brightness(image *img, float factor);
void apply_color(image *img, u8 color[4]);
void gamma_correction(image *img, float gamma);

// Geometric transformations
image *crop_image(const image *src, u16 x, u16 y, u16 width, u16 height);
image *rotate_image(const image *src, i8 clockwise_rotations);
image *flip_image_horizontal(const image *src);
image *flip_image_vertical(const image *src);

// Compositing
// void blend_images(image *dst, const image *src, float ratio);
void overlay_image(image *dst, const image *src, u16 x, u16 y);

// Utility functions
void clear_image(image *img);
void fill_color(image *img, u8 color[4]);
void get_avg_color_noalpha(image *img, u8 color_out[4]);

#endif