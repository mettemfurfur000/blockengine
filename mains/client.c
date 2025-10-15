#include "SDL_events.h"
#include "SDL_timer.h"
#include "SDL_video.h"

#include "include/events.h"
#include "include/logging.h"
#include "include/rendering.h"
#include "include/scripting.h"
#include "include/sdl2_basics.h"
#include "include/sdl2_extras.h"

// #include <time.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    log_start("client.log");

    if (init_graphics(false) != SUCCESS)
        return 1;

    unsigned long frame = 0;

    const int target_fps = FPS;
    const int ms_per_frame = 1000 / target_fps;

    const int target_tps = TPS;
    const int tick_period = 1000 / target_tps;

    client_render_rules rules = {.screen_height = SCREEN_HEIGHT, .screen_width = SCREEN_WIDTH};

    scripting_init();

    LUA_SET_GLOBAL_OBJECT("g_render_rules", &rules);

    scripting_load_file("engine", "init.lua");

    SDL_Event e;

    // clock_t latest_logic_tick = clock();
    u32 latest_logic_tick = SDL_GetTicks();

    e.type = ENGINE_INIT;
    call_handlers(e);

    float total_ms_took = 0;
    float total_ms_took_gpu = 0;

    const u32 perf_check_period_frames = 120;
    const float perf_ms_per_check = perf_check_period_frames * ms_per_frame;

    // e.type = SDL_RENDER_TARGETS_RESET;
    // SDL_PushEvent(&e);

    bool isFullscreen = false;

    for (;;)
    {
        // LOG_INFO("frame %lu", frame);
        u32 frame_begin_tick = SDL_GetTicks();

        while (SDL_PollEvent(&e))
        {
            switch (e.type) // vanilla sdl event handling
            {
            // case SDL_MOUSEBUTTONDOWN:
            //     LOG_DEBUG("retard");
            //     break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_F11)
                {
                    isFullscreen = !isFullscreen;
                    set_fullscreen(&rules, isFullscreen);
                }
                break;
            // case SDL_KEYUP:
            // case SDL_MOUSEBUTTONDOWN:
            // case SDL_MOUSEBUTTONUP:
            // break;
            case SDL_QUIT:
                call_handlers(e);
                goto logic_exit;
                break;
            case SDL_RENDER_TARGETS_RESET:
                // SDL_GetWindowSize(g_window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

                // LOG_DEBUG("changed resolution to %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
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

        {
            GLuint queryID;
            glGenQueries(1, &queryID);

            glBeginQuery(GL_TIME_ELAPSED, queryID);

            client_render(rules);
            SDL_GL_SwapWindow(g_window);

            glEndQuery(GL_TIME_ELAPSED);

            // Retrieve the result (can block until available)
            GLint available = 0;
            while (!available)
            {
                glGetQueryObjectiv(queryID, GL_QUERY_RESULT_AVAILABLE, &available);
            }
            GLuint64 timeElapsed;
            glGetQueryObjectui64v(queryID, GL_QUERY_RESULT, &timeElapsed);

            total_ms_took_gpu += timeElapsed / 1000000.0f;

            // 'timeElapsed' will contain the duration in nanoseconds
            glDeleteQueries(1, &queryID);
        }

        int loop_took = SDL_GetTicks() - frame_begin_tick;
        int chill_time = ms_per_frame - loop_took;

        total_ms_took += loop_took;
        const int sleep_final = chill_time < 1 ? 1 : chill_time;

        SDL_Delay(sleep_final);

        if (frame % perf_check_period_frames == 0 && frame != 0)
        {
            LOG_INFO("perf: %f%% cpu, gpu: %f%%", total_ms_took / perf_ms_per_check,
                     total_ms_took_gpu / perf_ms_per_check);

            total_ms_took = 0;
            total_ms_took_gpu = 0;
        }

        frame++;
    }
logic_exit:
    LOG_INFO("exiting...");

    exit_graphics();

    scripting_close();

    return 0;
}