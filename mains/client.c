#include "../include/events.h"
#include "../include/rendering.h"
#include "../include/scripting.h"

int main(int argc, char *argv[])
{
    log_start("client.log");

    if (init_graphics() != SUCCESS)
        return 1;

    unsigned long frame = 0;

    const int target_fps = 60;
    const int ms_per_s = 1000 / target_fps;

    const int target_tps = 10;
    const int tick_period = 1000 / target_tps;

    client_render_rules rules = {.screen_height = SCREEN_HEIGHT,
                                 .screen_width = SCREEN_WIDTH

    };

    scripting_init();

    LUA_SET_GLOBAL_OBJECT("g_render_rules", &rules);

    scripting_load_file("engine", "init.lua");

    SDL_Event e;

    int latest_logic_tick = SDL_GetTicks();

    e.type = ENGINE_INIT;
    call_handlers(e);

    u32 total_ms_took = 0;
    u32 perf_report_start = SDL_GetTicks();

    for (;;)
    {
        // LOG_INFO("frame %lu", frame);
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

            // scripting event handling
            call_handlers(e);
        }

        if (latest_logic_tick + tick_period <= frame_begin_tick)
        {
            latest_logic_tick = SDL_GetTicks();
            e.type = ENGINE_TICK;
            call_handlers(e);
        }

        glClearColor(0.7f, 0.7f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        client_render(rules);

        SDL_GL_SwapWindow(g_window);

        int loop_took = SDL_GetTicks() - frame_begin_tick;
        int chill_time = ms_per_s - loop_took;
        total_ms_took += loop_took;

        SDL_Delay(fmax(1, chill_time));

        if (frame % 600 == 0)
        {
            u32 ms_since_report = SDL_GetTicks() - perf_report_start;
            LOG_INFO("perf: %dms / %dms", total_ms_took, ms_since_report);
            total_ms_took = 0;

            perf_report_start = SDL_GetTicks();
        }

        frame++;
    }
logic_exit:
    LOG_INFO("exiting...");

    exit_graphics();

    scripting_close();

    return 0;
}