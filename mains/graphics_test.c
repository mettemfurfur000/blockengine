#include "../include/vars.h"
#include "../include/level.h"
#include "../include/rendering.h"
#include "../include/block_registry.h"
#include "../include/scripting.h"

#include <time.h>

int main(int argc, char *argv[])
{
    log_start("client.log");

    if (init_graphics() == FAIL)
        return 1;

    unsigned long frame = 0;

    const int target_fps = 60;
    const int ms_per_s = 1000 / target_fps;

    SDL_Event e;

    int latest_logic_tick = SDL_GetTicks();

    // C:\msys64\home\mttffr\blockengine\build\registries\test\textures

    const char *testfile = "registries/test/textures/stone.png";

    texture tmp = {};
    texture_load(&tmp, testfile);

    for (;;)
    {
        int frame_begin_tick = SDL_GetTicks();

        while (SDL_PollEvent(&e))
        {
            switch (e.type) // vanilla sdl event handling
            {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                break;
            case SDL_QUIT:
                goto logic_exit;
                break;
            case SDL_RENDER_TARGETS_RESET:
                // g_renderer = SDL_CreateRenderer(g_window, -1, 0);
                // printf("erm, reload textures?");
                break;
            }
        }

        glClearColor(0.1f, 0.35f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        block_render(&tmp, 0, 0, 0, 0, 0, g_block_width, 0, 0);
        
        SDL_GL_SwapWindow(g_window);

        int loop_took = SDL_GetTicks() - frame_begin_tick;
        int chill_time = ms_per_s - loop_took;

        SDL_Delay(max(1, chill_time));

        frame++;
    }
logic_exit:
    LOG_INFO("exiting...");

    exit_graphics();

    return 0;
}