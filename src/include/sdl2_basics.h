#ifndef SDL2_BASICS
#define SDL2_BASICS

#include <SDL2/SDL.h>

#include "engine_types.h"

#include "../../stb/stb_image.h"

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern char *window_name;

extern SDL_Window *g_window;
extern SDL_Renderer *g_renderer;

extern int g_block_width;

#define TEXTURE_TYPE_REGULAR 1
#define TEXTURE_TYPE_CONNECTED 2

typedef struct texture
{
	SDL_Texture *ptr;
	char *filename;

	int width;
	int height;

	unsigned short frames;		 // amount of vertical frames in a texture
	unsigned short types;		 // amount of types/states of a texture with their unique animations
	unsigned short total_frames; // things above multiplied
} texture;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define KEEPINLIMITS(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

int greatest_common_divisor(int a, int b);
char *get_folder_path(char *file_path, int bonus_for_str_size);

int init_graphics();
int exit_graphics();

int texture_load(texture *dest, char *path_to_file);
void free_texture(texture *t);

int block_render(texture *texture, const int x, const int y, byte frame, byte type, byte ignore_type, byte local_block_width, byte flip, unsigned short rotation);

#endif