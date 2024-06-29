#ifndef SDL2_BASICS
#define SDL2_BASICS

#include <SDL2/SDL.h>

#include "game_types.h"

#include "../../stb/stb_image.h"

extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const char *window_name;
extern SDL_Window *g_window;
extern SDL_Renderer *g_renderer;

typedef struct texture
{
	SDL_Texture *ptr;
	char *filename;

	int width;
	int height;

	int frame_side_size;

	int frames_per_line;
	int frames;
} texture;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define KEEPINLIMITS(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

int greatest_common_divisor(int a, int b);

char *get_folder_path(char *file_path, int bonus_for_str_size);

int init_graphics();

int exit_graphics();

int handle_events();

int texture_load(texture *dest, char *path_to_file);

void free_texture(texture *t);

int texture_render(texture *texture, int x, int y, float scale);

int texture_render_anim(texture *texture, int x, int y, int frame, float scale);

#endif