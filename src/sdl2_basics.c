#include <SDL2/SDL.h>
#include "game_types.h"

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window *window = NULL;

SDL_Surface *screenSurface = NULL;

int init_graphics_silent()
{
    const int flags = SDL_INIT_VIDEO;
    int status = SUCCESS;

    status &= !SDL_Init(flags);

    window = SDL_CreateWindow("block engine test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    status &= (int)window;

    screenSurface = SDL_GetWindowSurface(window);

    status &= (int)screenSurface;

    return status;
}

int black_frame()
{
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

    SDL_UpdateWindowSurface(window);
}

int exit_graphics()
{
    SDL_DestroyWindow(window);

    SDL_Quit();
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