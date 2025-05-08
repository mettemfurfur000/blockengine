// #include "../include/block_registry.h"
// #include "../include/level.h"
// #include "../include/rendering.h"
// #include "../include/scripting.h"
// #include "../include/vars.h"

#include "../include/sdl2_basics.h"
#include "../include/block_renderer.h"

// #include <time.h>

int main(int argc, char *argv[])
{
    log_start("client.log");

    if (init_graphics() != SUCCESS)
        return 1;

    const int target_fps = 60;
    const int ms_per_s = 1000 / target_fps;

    SDL_Event e;

    // C:\msys64\home\mttffr\blockengine\build\registries\test\textures

    char *testfile = "registries/engine/textures/bug.png";

    texture tmp = {};
    texture_load(&tmp, testfile);
    glClearColor(0.7f, 0.7f, 0.6f, 1.0f);

    for (;;)
    {
        int frame_begin_tick = SDL_GetTicks();

        while (SDL_PollEvent(&e))
        {
            switch (e.type) // vanilla sdl event handling
            {
            case SDL_QUIT:
                goto logic_exit;
                break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        // block_render(&tmp, 0, 0, 0, 0, 0, g_block_width * 4, 0, 0);

        SDL_GL_SwapWindow(g_window);

        int loop_took = SDL_GetTicks() - frame_begin_tick;
        int chill_time = ms_per_s - loop_took;

        SDL_Delay(fmax(1, chill_time));
    }
logic_exit:
    LOG_INFO("exiting...");

    exit_graphics();

    return 0;
}