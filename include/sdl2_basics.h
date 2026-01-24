#ifndef SDL2_BASICS_H
#define SDL2_BASICS_H 1

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <epoxy/gl.h>

#include "general.h"
// #include "rendering.h"

#include "vec/src/vec.h"
// #include "../stb/stb_image.h"
#include "image_editing.h"

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern char *window_name;

extern SDL_Window *g_window;
extern SDL_GLContext *g_gl_context;

extern int g_block_width;

#define TEXTURE_TYPE_REGULAR 1
#define TEXTURE_TYPE_CONNECTED 2

typedef struct atlas_info
{
    u32 atlas_offset_x, atlas_offset_y;

    i32 width;
    i32 height;

    u8 frames;       // frames in an animation, in height
    u8 types;        // amount of animations, in width
    u8 total_frames; // things above multiplied
} atlas_info;

typedef struct sound
{
    char *filename;
    Mix_Chunk *obj;

    u32 length_ms;
} sound;

typedef vec_t(sound) vec_sound_t;

typedef struct music
{
    char *filename;
    Mix_Music *mus;
    u8 type;

    u32 length_ms;
    u8 channels;
} music;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define KEEPINLIMITS(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

int greatest_common_divisor(int a, int b);

int init_graphics(bool set_fullscreen);
int exit_graphics();

int record_atlas_info(atlas_info *atlas_info, image *img);

int sound_load(sound *dest, char *filename);
void free_sound(sound *s);

int music_load(music *dest, char *filename);
void free_music(music *s);

#endif