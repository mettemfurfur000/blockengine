#include "src/sdl2_basics.c"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (!init_graphics())
        return 1;

    // black_frame();

    basic_texture test = {{0, 0}, 0};

    texture_load(&test, "test.png");

    double i = 0;

    while (handle_events())
    {
        SDL_RenderClear(g_renderer);

        texture_render(&test, 64 + sin(i) * 64, 64 + cos(i) * 64);

        SDL_RenderPresent(g_renderer);
        SDL_Delay(16);

        i += M_PI / 90;
    }

    exit_graphics();
    return 0;
}