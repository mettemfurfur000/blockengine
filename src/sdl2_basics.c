#include "../include/sdl2_basics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
char *window_name = "Block Engine";

SDL_Window *g_window = NULL;
SDL_Renderer *g_renderer = NULL;

int g_block_width = 16;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define KEEPINLIMITS(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

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

	int result = SDL_RegisterEvents(TOTAL_ENGINE_EVENTS);
	status &= result == (Uint32)-1 ? 0 : 1;

	if (!status)
		printf("Not able to allocate enough user events\n");

	return status;
}

int exit_graphics()
{
	SDL_DestroyWindow(g_window);

	SDL_DestroyRenderer(g_renderer);

	SDL_Quit();

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

	u8 *image_data;
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

	dest->frames = dest->width / g_block_width;
	dest->types = dest->height / g_block_width;
	dest->total_frames = dest->frames * dest->types;

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

// it trusts you to pass valid values in, be careful with frames and types...
int block_render(texture *texture, const int x, const int y, u8 frame, u8 type, u8 ignore_type, u8 local_block_width, u8 flip, unsigned short rotation)
{
	frame = frame % texture->total_frames;

	int frame_x = (frame % texture->frames) * g_block_width;
	int frame_y = 0;

	if (ignore_type)
		frame_y = (frame / texture->frames) * g_block_width;
	else
		frame_y = (type % texture->types) * g_block_width;

	SDL_Rect src = {frame_x, frame_y, g_block_width, g_block_width};
	SDL_Rect dest = {x, y, local_block_width, local_block_width};

	const SDL_Point center = {local_block_width / 2, local_block_width / 2};

	// return !SDL_RenderCopy(g_renderer, texture->ptr, &src, &dest);

	return !SDL_RenderCopyEx(g_renderer, texture->ptr, &src, &dest, rotation, &center, flip);
}

// TODO:

/*
neighbours_mask expected layout, bit by bit:
 0 1 2
 3 X 4
 5 6 7

 X is block being rendered
 if bit is 1, then neighbour have same block id
*/

// int block_render_connected(texture *texture, int x, int y, u8 neighbours_mask)
// {
// 	const u8 half_w = g_block_width / 2;
// 	const u8 quart_w = g_block_width / 4;
// 	const u8 three_quarts_w = half_w + quart_w;

// 	SDL_Rect src = {0, 0, half_w, half_w};
// 	SDL_Rect dest = {x + quart_w, y + quart_w, half_w, half_w};
// 	SDL_Point rotation_center = {0, 0};

// 	SDL_RenderCopy(g_renderer, texture->ptr, &src, &dest);

// 	for (int i = 1; i < 7; i += 2)
// 	{
// 		if (i == 5)
// 			i--;

// 		double angle;

// 		switch (i)
// 		{
// 		case 1:
// 			angle = 90;
// 			break;
// 		case 3:
// 			angle = 180;
// 			break;
// 		case 4:
// 			angle = 0;
// 			break;
// 		case 6:
// 			angle = 270;
// 			break;
// 		}

// 		src.x = (neighbours_mask & (0b10000000 >> i)) ? half_w : half_w + quart_w;
// 		src.y = 0;
// 		src.h = half_w;
// 		src.w = quart_w;

// 		dest.x = x + (i == 1 || i == 6 ? quart_w
// 					  : i == 4		   ? three_quarts_w
// 									   : 0);
// 		dest.y = y + (i == 1   ? 0
// 					  : i == 6 ? three_quarts_w
// 							   : quart_w);
// 		dest.h = half_w;
// 		dest.w = quart_w;

// 		rotation_center.x = 0;
// 		rotation_center.y = 0;

// 		SDL_RenderCopyEx(g_renderer, texture->ptr, &src, &dest, angle, &rotation_center, SDL_FLIP_NONE);
// 	}
// }
