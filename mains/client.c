#include "../include/events.h"
#include "../include/rendering.h"
#include "../include/scripting.h"
#include "SDL_events.h"

#include <time.h>

int main(int argc, char *argv[])
{
    log_start("client.log");

    if (init_graphics() != SUCCESS)
        return 1;

    unsigned long frame = 0;

    const int target_fps = FPS;
    const int ms_per_s = 1000 / target_fps;

    const int target_tps = TPS;
    const int tick_period = 1000 / target_tps;

    client_render_rules rules = {.screen_height = SCREEN_HEIGHT, .screen_width = SCREEN_WIDTH};

    scripting_init();

    LUA_SET_GLOBAL_OBJECT("g_render_rules", &rules);

    scripting_load_file("engine", "init.lua");

    SDL_Event e;

    clock_t latest_logic_tick = clock();

    e.type = ENGINE_INIT;
    call_handlers(e);

    u32 total_ms_took = 0;
    const u32 perf_check_each = 600;

    for (;;)
    {
        // LOG_INFO("frame %lu", frame);
        int frame_begin_tick = clock();

        while (SDL_PollEvent(&e))
        {
            switch (e.type) // vanilla sdl event handling
            {
            // case SDL_MOUSEBUTTONDOWN:
            //     LOG_DEBUG("retard");
            //     break;
            // case SDL_KEYDOWN:
            // case SDL_KEYUP:
            // case SDL_MOUSEBUTTONDOWN:
            // case SDL_MOUSEBUTTONUP:
            // break;
            case SDL_QUIT:
                call_handlers(e);
                goto logic_exit;
                break;
            case SDL_RENDER_TARGETS_RESET:
                // g_renderer = SDL_CreateRenderer(g_window, -1, 0);
                // printf("erm, reload textures?");
                break;
            }

            // scripting event handling
            call_handlers(e);
        }

        if (latest_logic_tick + tick_period <= frame_begin_tick)
        {
            latest_logic_tick = clock();
            e.type = ENGINE_TICK;
            call_handlers(e);
        }

        client_render(rules);

        SDL_GL_SwapWindow(g_window);

        int loop_took = clock() - frame_begin_tick;
        int chill_time = ms_per_s - loop_took;
        total_ms_took += loop_took;

        const int sleep_final = chill_time < 1 ? 1 : chill_time;
        usleep(sleep_final * 1000);

        if (frame % perf_check_each == 0 && frame != 0)
        {
            LOG_INFO("perf: %f%% for %d frames", (1.0f * perf_check_each) / total_ms_took, perf_check_each);
            total_ms_took = 0;
        }

        frame++;
    }
logic_exit:
    LOG_INFO("exiting...");

    exit_graphics();

    scripting_close();

    return 0;
}