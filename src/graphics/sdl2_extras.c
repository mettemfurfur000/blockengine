#include "include/sdl2_extras.h"
#include "include/block_renderer.h"
#include "include/sdl2_basics.h"

void set_fullscreen(client_render_rules *rules, bool set_full)
{
    SDL_SetWindowFullscreen(g_window, set_full ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_GetWindowSize(g_window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

    printf("Got window size %d x %d for fullscreen\n", SCREEN_WIDTH, SCREEN_HEIGHT);

    rules->screen_height = SCREEN_HEIGHT;
    rules->screen_width = SCREEN_WIDTH;

    for (u32 i = 0; i < rules->slices.length; i++)
    {
        rules->slices.data[i].h = SCREEN_HEIGHT;
        rules->slices.data[i].w = SCREEN_WIDTH;
    }

    block_renderer_update_size();
}