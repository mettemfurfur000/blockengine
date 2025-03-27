#ifndef SDL2_BASICS_H
#define SDL2_BASICS_H 1

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_mixer.h>

#include "general.h"
#include "events.h"
#include "../vec/src/vec.h"

#include "../stb/stb_image.h"

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern char *window_name;

extern SDL_Window *g_window;
// extern SDL_Renderer *g_renderer; // do not use
extern SDL_GLContext *g_gl_context;

extern int g_block_width;

#define TEXTURE_TYPE_REGULAR 1
#define TEXTURE_TYPE_CONNECTED 2

typedef struct texture
{
	// SDL_Texture *ptr;
	GLuint gl_id;
	char *filename;

	int width;
	int height;

	unsigned short frames;		 // amount of vertical frames in a texture
	unsigned short types;		 // amount of types/states of a texture with their unique animations
	unsigned short total_frames; // things above multiplied
} texture;

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
char *get_folder_path(char *file_path, int bonus_for_str_size);

int init_graphics();
int exit_graphics();

int texture_load(texture *dest, char *path_to_file);
void free_texture(texture *t);

int sound_load(sound *dest, char *filename);
void free_sound(sound *s);

int music_load(music *dest, char *filename);
void free_music(music *s);

void play_sound_randomly(vec_sound_t sounds);

int block_render(texture *texture, const int x, const int y, u8 frame, u8 type, u8 ignore_type, u8 local_block_width, u8 flip, unsigned short rotation);

#endif