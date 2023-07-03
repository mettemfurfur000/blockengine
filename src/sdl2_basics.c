#ifndef SDL2_BASICS_H
#define SDL2_BASICS_H 1

#include <SDL2/SDL.h>
#include "game_types.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const char *window_name = "Block Engine";

SDL_Window *g_window = NULL;

SDL_Surface *g_screen = NULL;

SDL_Renderer *g_renderer = NULL;

typedef struct basic_texture
{
    SDL_Point sizes;
    SDL_Texture *ptr;
} basic_texture;

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

int texture_load(basic_texture *dest, char *path_to_file)
{
    if (!dest)
        return FAIL;
    if (!path_to_file)
        return FAIL;

    int width, height, channels;
    unsigned char *image_data = stbi_load(path_to_file, &width, &height, &channels, STBI_rgb_alpha);

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(image_data, width, height, 32, width * 4, 0xff, 0xff00, 0xff0000, 0xff000000);

    stbi_image_free(image_data);

    if (!surface)
        return FAIL;

    SDL_Texture *texture_ptr = SDL_CreateTextureFromSurface(g_renderer, surface);

    if (!texture_ptr)
        return FAIL;

    dest->ptr = texture_ptr;
    dest->sizes.x = width;
    dest->sizes.y = height;

    SDL_FreeSurface(surface);

    return SUCCESS;
}

int texture_render(basic_texture *texture, int x, int y)
{
    SDL_Rect dest = {x, y, texture->sizes.x, texture->sizes.y};
    return !SDL_RenderCopy(g_renderer, texture->ptr, NULL, &dest);
}

#endif