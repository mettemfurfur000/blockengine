#ifndef SDL2_BASICS_H
#define SDL2_BASICS_H 1

#include <SDL2/SDL.h>
#include "game_types.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const char *window_name = "Block Engine";

// g_ == Global, rember?

SDL_Window *g_window = NULL;

SDL_Renderer *g_renderer = NULL;

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

int greatest_common_divisor(int a, int b)
{
	int temp;
	while (b != 0)
	{
		temp = b;
		b = a % b;
		a = temp;
	}
	return a;
}

char *get_folder_path(char *file_path, int bonus_for_str_size)
{
	// from you.com/chat
	int len = strlen(file_path);
	char *folder_path = (char *)malloc(len + bonus_for_str_size);
	strcpy(folder_path, file_path);
	// find the last occurrence of '/'
	char *last_slash = strrchr(folder_path, '/');
	if (last_slash != NULL)
	{
		// if there is a slash, terminate the string after it
		*last_slash = '\0';
	}
	else
	{
		// if there is no slash, return NULL
		free(folder_path);
		return NULL;
	}

	return folder_path;
}

int init_graphics()
{
	const int flags = SDL_INIT_VIDEO;

	int status = SUCCESS;
	status &= !SDL_Init(flags);

	g_window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	status &= g_window ? 1 : 0;

	g_renderer = SDL_CreateRenderer(g_window, -1, 0);
	status &= g_renderer ? 1 : 0;

	if (!status)
		printf("init_graphics() error:[%s]\n", SDL_GetError());

	SDL_SetRenderDrawColor(g_renderer, 0xFF, 0xFF, 0xFF, 0xFF);

	return status;
}

int exit_graphics()
{
	SDL_DestroyWindow(g_window);

	SDL_DestroyRenderer(g_renderer);

	SDL_Quit();

	return SUCCESS;
}

int handle_events()
{
	SDL_Event e;

	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_QUIT)
			return FAIL;
	}

	return SUCCESS;
}

int texture_load(texture *dest, char *path_to_file)
{
	if (!dest)
	{
		printf("texture_load Error: No desination texture\n");
		return FAIL;
	}

	if (!path_to_file)
	{
		printf("texture_load Error: No path to file\n");
		return FAIL;
	}

	byte *image_data;
	int channels;

	if (!(image_data = stbi_load(path_to_file, &dest->width, &dest->height, &channels, STBI_rgb_alpha)))
	{
		printf("texture_load Error: stbi_load failed, no such file: %s\n", path_to_file);
		return FAIL;
	}

	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(image_data, dest->width, dest->height, 32, dest->width * 4, 0xff, 0xff00, 0xff0000, 0xff000000);

	if (!surface)
	{
		printf("texture_load Error: SDL_CreateRGBSurfaceFrom failed\n");
		return FAIL;
	}

	dest->ptr = SDL_CreateTextureFromSurface(g_renderer, surface);

	SDL_FreeSurface(surface);

	if (!dest->ptr)
	{
		printf("texture_load Error: SDL_CreateTextureFromSurface failed\n");
		printf("%s\n", SDL_GetError());
		return FAIL;
	}

	// animation data calculations

	dest->frame_side_size = greatest_common_divisor(dest->height, dest->width);
	dest->frames_per_line = dest->width / dest->frame_side_size;
	dest->frames = dest->frames_per_line * (dest->height / dest->frame_side_size);

	// copy filename
	char *filename = strrchr(path_to_file, '/') + 1;
	int namelen = strlen(filename);

	dest->filename = (char *)malloc(namelen + 1);

	strcpy(dest->filename, filename);

	return SUCCESS;
}

void free_texture(texture *t)
{
	if (t->filename)
	{
		free(t->filename);
		t->filename = 0;
	}
	if (t->ptr)
	{
		SDL_DestroyTexture(t->ptr);
		t->ptr = 0;
	}
}

int texture_render(texture *texture, int x, int y, float scale)
{
	SDL_Rect src = {0, 0, texture->frame_side_size, texture->frame_side_size};
	SDL_Rect dest = {x, y, texture->frame_side_size * scale, texture->frame_side_size * scale};

	return !SDL_RenderCopy(g_renderer, texture->ptr, &src, &dest);
}

int texture_render_anim(texture *texture, int x, int y, int frame, float scale)
{
	frame = frame % texture->frames;
	int frame_x = texture->frame_side_size * (frame % texture->frames_per_line);
	int frame_y = texture->frame_side_size * (frame / texture->frames_per_line);

	SDL_Rect src = {frame_x, frame_y, texture->frame_side_size, texture->frame_side_size};
	SDL_Rect dest = {x, y, texture->frame_side_size * scale, texture->frame_side_size * scale};
	
	return !SDL_RenderCopy(g_renderer, texture->ptr, &src, &dest);
}

#endif