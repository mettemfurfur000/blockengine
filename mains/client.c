#include "SDL_events.h"
#include "SDL_timer.h"
#include "SDL_video.h"

#include "include/events.h"
#include "include/logging.h"
#include "include/rendering.h"
#include "include/scripting.h"
#include "include/sdl2_basics.h"
#include "include/sdl2_extras.h"

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
    const char *log_output;  // "stdout" or filename (default: "client.log")
    int screen_width;        // Screen width (default: SCREEN_WIDTH)
    int screen_height;       // Screen height (default: SCREEN_HEIGHT)
    int target_fps;          // Target FPS (default: FPS)
    int target_tps;          // Target TPS (default: TPS)
    int fullscreen;          // 1 for fullscreen, 0 for windowed, -1 for default
    const char *registry;    // Registry path (default: "engine")
    const char *init_script; // Init script file (default: "init.lua")
    int enable_perf_checks;  // 1 to enable, 0 to disable, -1 for default
} ClientConfig;

static void print_usage(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -l, --log <output>        Log output: 'stdout' or filename (default: client.log)\n");
    printf("  -w, --width <pixels>      Screen width (default: %d)\n", SCREEN_WIDTH);
    printf("  -h, --height <pixels>     Screen height (default: %d)\n", SCREEN_HEIGHT);
    printf("  -f, --fps <rate>          Target FPS (default: %d)\n", FPS);
    printf("  -t, --tps <rate>          Target TPS (default: %d)\n", TPS);
    printf("  -F, --fullscreen          Start in fullscreen mode\n");
    printf("  -W, --windowed            Start in windowed mode\n");
    printf("  -r, --registry <path>     Registry path (default: engine)\n");
    printf("  -s, --script <file>       Init script file (default: init.lua)\n");
    printf("  -p, --perf-checks         Enable performance checks (default: enabled)\n");
    printf("  -P, --no-perf-checks      Disable performance checks\n");
    printf("  --help                    Show this help message\n");
}

static ClientConfig parse_arguments(int argc, char *argv[])
{
    ClientConfig config = {.log_output = "client.log",
                           .screen_width = SCREEN_WIDTH,
                           .screen_height = SCREEN_HEIGHT,
                           .target_fps = FPS,
                           .target_tps = TPS,
                           .fullscreen = -1,
                           .registry = "engine",
                           .init_script = "init.lua",
                           .enable_perf_checks = -1};

    static struct option long_options[] = {
        {           "log", required_argument, 0, 'l'},
        {         "width", required_argument, 0, 'w'},
        {        "height", required_argument, 0, 'h'},
        {           "fps", required_argument, 0, 'f'},
        {           "tps", required_argument, 0, 't'},
        {    "fullscreen",       no_argument, 0, 'F'},
        {      "windowed",       no_argument, 0, 'W'},
        {      "registry", required_argument, 0, 'r'},
        {        "script", required_argument, 0, 's'},
        {   "perf-checks",       no_argument, 0, 'p'},
        {"no-perf-checks",       no_argument, 0, 'P'},
        {          "help",       no_argument, 0, 'H'},
        {               0,                 0, 0,   0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "l:w:h:f:t:FWr:s:pPH", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'l':
            config.log_output = optarg;
            break;
        case 'w':
            config.screen_width = atoi(optarg);
            if (config.screen_width < 800)
            {
                fprintf(stderr, "Warning: width too small, setting to 800\n");
                config.screen_width = 800;
            }
            break;
        case 'h':
            config.screen_height = atoi(optarg);
            if (config.screen_height < 600)
            {
                fprintf(stderr, "Warning: height too small, setting to 600\n");
                config.screen_height = 600;
            }
            break;
        case 'f':
            config.target_fps = atoi(optarg);
            if (config.target_fps < 1)
            {
                fprintf(stderr, "Warning: FPS must be at least 1\n");
                config.target_fps = 1;
            }
            break;
        case 't':
            config.target_tps = atoi(optarg);
            if (config.target_tps < 1)
            {
                fprintf(stderr, "Warning: TPS must be at least 1\n");
                config.target_tps = 1;
            }
            break;
        case 'F':
            config.fullscreen = 1;
            break;
        case 'W':
            config.fullscreen = 0;
            break;
        case 'r':
            config.registry = optarg;
            break;
        case 's':
            config.init_script = optarg;
            break;
        case 'p':
            config.enable_perf_checks = 1;
            break;
        case 'P':
            config.enable_perf_checks = 0;
            break;
        case 'H':
            print_usage(argv[0]);
            exit(0);
        case '?':
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            exit(1);
        default:
            break;
        }
    }

    return config;
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam)
{
    if (type != 0x8251)
        LOG_ERROR("GL: %s type = 0x%x, severity = 0x%x, message = %s",
                  (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

int main(int argc, char *argv[])
{
    ClientConfig config = parse_arguments(argc, argv);

    SCREEN_WIDTH = config.screen_width;
    SCREEN_HEIGHT = config.screen_height;

    log_start(config.log_output);

    init_backtrace();

    if (init_graphics(false) != SUCCESS)
        return 1;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    unsigned long frame = 0;

    const int target_fps = config.target_fps;
    const int ms_per_frame = 1000 / target_fps;

    const int target_tps = config.target_tps;
    const int tick_period = 1000 / target_tps;

    client_render_rules rules = {.screen_height = config.screen_height, .screen_width = config.screen_width};

    scripting_init();

    LUA_SET_GLOBAL_OBJECT("g_render_rules", &rules);

    scripting_load_file(config.registry, config.init_script);

    SDL_Event e;
    u32 latest_logic_tick = SDL_GetTicks();

    e.type = ENGINE_INIT;
    call_handlers(e);

    float total_ms_took = 0;

    const u32 perf_check_period_frames = 600;
    const float perf_ms_per_check = perf_check_period_frames * ms_per_frame;

    // Determine if perf checks are enabled
    int perf_checks_enabled = (config.enable_perf_checks == -1) ? 1 : config.enable_perf_checks;

    bool isFullscreen = (config.fullscreen == 1) ? true : false;
    if (config.fullscreen != -1)
    {
        set_fullscreen(&rules, isFullscreen);
    }

    for (;;)
    {
        u32 frame_begin_tick = SDL_GetTicks();

        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_F11)
                {
                    isFullscreen = !isFullscreen;
                    set_fullscreen(&rules, isFullscreen);
                }
                break;
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

        client_render(rules);
        SDL_GL_SwapWindow(g_window);

        int loop_took = SDL_GetTicks() - frame_begin_tick;
        int chill_time = ms_per_frame - loop_took;

        total_ms_took += loop_took;
        const int sleep_final = chill_time < 1 ? 1 : chill_time;

        SDL_Delay(sleep_final);

        if (perf_checks_enabled && frame % perf_check_period_frames == 0 && frame != 0)
        {
            LOG_INFO("perf: %f%% cpu", total_ms_took / perf_ms_per_check);
            total_ms_took = 0;
        }

        frame++;
    }
logic_exit:
    LOG_INFO("exiting...");

    deinit_backtrace();

    exit_graphics();

    scripting_close();

    return 0;
}