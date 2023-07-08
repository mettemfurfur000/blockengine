#ifndef SDL2_BASICS_H
#define SDL2_BASICS_H 1

#include <SDL2/SDL.h>
#include "block_properties.c"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const char *window_name = "Block Engine";

SDL_Window *g_window = NULL;

SDL_Surface *g_screen = NULL;

SDL_Renderer *g_renderer = NULL;

typedef struct texture
{
    SDL_Texture *ptr;

    int width;
    int height;

    int frame_side_size;

    int frames_per_line;
    int frames;
} texture;

int gcd(int a, int b)
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

    g_screen = SDL_GetWindowSurface(g_window);
    status &= g_screen ? 1 : 0;

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    status &= g_renderer ? 1 : 0;

    SDL_SetRenderDrawColor(g_renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    if (!status)
        printf("woops:[%s]\n", SDL_GetError());

    return status;
}

int black_frame()
{
    SDL_FillRect(g_screen, NULL, SDL_MapRGB(g_screen->format, 0xFF, 0xFF, 0xFF));

    return !SDL_UpdateWindowSurface(g_window);
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
        return FAIL;
    if (!path_to_file)
        return FAIL;

    unsigned char *image_data;
    int channels;

    if (!(image_data = stbi_load(path_to_file, &dest->width, &dest->height, &channels, STBI_rgb_alpha)))
        return FAIL;

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(image_data, dest->width, dest->height, 32, dest->width * 4, 0xff, 0xff00, 0xff0000, 0xff000000);

    dest->ptr = SDL_CreateTextureFromSurface(g_renderer, surface);

    SDL_FreeSurface(surface);

    if (!dest->ptr)
        return FAIL;

    // animation data calculations

    dest->frame_side_size = gcd(dest->height, dest->width);
    dest->frames_per_line = dest->width / dest->frame_side_size;
    dest->frames = dest->frames_per_line * (dest->height / dest->frame_side_size);

    return SUCCESS;
}

int texture_render(texture *texture, int x, int y, float scale)
{
    SDL_Rect src = {0, 0, texture->frame_side_size, texture->frame_side_size};
    SDL_Rect dest = {x, y, texture->frame_side_size*scale, texture->frame_side_size*scale};
    return !SDL_RenderCopy(g_renderer, texture->ptr, &src, &dest);
}

int texture_render_anim(texture *texture, int x, int y, int frame, float scale)
{
    frame = frame % texture->frames;
    int frame_x = texture->frame_side_size * (frame % texture->frames_per_line);
    int frame_y = texture->frame_side_size * (frame / texture->frames_per_line);

    SDL_Rect src = {frame_x, frame_y, texture->frame_side_size, texture->frame_side_size};

    SDL_Rect dest = {x, y, texture->frame_side_size, texture->frame_side_size};
    return !SDL_RenderCopy(g_renderer, texture->ptr, &src, &dest);
}

#endif