#include "include/block_registry.h"
#include "include/events.h"
#include "include/logging.h"
#include "include/network.h"
#include "include/scripting.h"
#include "include/signal_handler.h"
// #include "include/level.h"
// #include "include/update_system.h"

#include <SDL_events.h>
#include <SDL_timer.h>

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
    const char *log_output;  // "stdout" or filename (default: "server.log")
    u16 port;                // Server port (default: 7777)
    int target_tps;          // Target TPS (default: TPS)
    const char *registry;    // Registry path (default: "engine")
    int enable_perf_checks;  // 1 to enable, 0 to disable, -1 for default
} ServerConfig;

static void print_usage(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -l, --log <output>        Log output: 'stdout' or filename\n");
    printf("  -p, --port <number>       Server port (default: 7777)\n");
    printf("  -t, --tps <rate>          Target TPS (default: %d)\n", TPS);
    printf("  -r, --registry <path>     Registry path (default: engine)\n");
    printf("  -P, --perf-checks         Enable performance checks (default: enabled)\n");
    printf("  -N, --no-perf-checks      Disable performance checks\n");
    printf("  --help                    Show this help message\n");
}

static ServerConfig parse_arguments(int argc, char *argv[])
{
    ServerConfig config = {.log_output = "default",
                           .port = 7777,
                           .target_tps = TPS,
                           .registry = "engine",
                           .enable_perf_checks = -1};

    static struct option long_options[] = {
        {         "log", required_argument, 0, 'l'},
        {        "port", required_argument, 0, 'p'},
        {         "tps", required_argument, 0, 't'},
        {    "registry", required_argument, 0, 'r'},
        { "perf-checks",       no_argument, 0, 'P'},
        {"no-perf-checks",     no_argument, 0, 'N'},
        {        "help",       no_argument, 0, 'h'},
        {             0,                 0, 0,   0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "l:p:t:r:PNh", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'l':
            config.log_output = optarg;
            break;
        case 'p':
            config.port = (u16)atoi(optarg);
            if (config.port == 0)
            {
                fprintf(stderr, "Warning: invalid port, using 7777\n");
                config.port = 7777;
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
        case 'r':
            config.registry = optarg;
            break;
        case 'P':
            config.enable_perf_checks = 1;
            break;
        case 'N':
            config.enable_perf_checks = 0;
            break;
        case 'h':
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

int main(int argc, char *argv[])
{
    ServerConfig config = parse_arguments(argc, argv);

    if (strcmp(config.log_output, "default") == 0)
        log_start_new("server");
    else
        log_start(config.log_output);

    init_backtrace();
    init_signal_handlers();

    // Initialize networking on the specified port
    if (net_server_init(config.port) != 0)
    {
        LOG_ERROR("Failed to initialize network server on port %u", config.port);
        return 1;
    }

    LOG_INFO("Network server initialized on port %u", config.port);

    scripting_init();

    // Load block registry
    block_registry *reg = registry_load(config.registry);
    if (reg == NULL)
    {
        LOG_ERROR("No registry found at: %s", config.registry);
        net_server_shutdown();
        return -1;
    }

    LUA_SET_GLOBAL_USER_OBJECT("G_block_registry", BlockRegistry, reg);

    unsigned long frame = 0;
    const int target_tps = config.target_tps;
    const int tick_period = 1000 / target_tps;

    u32 latest_logic_tick = SDL_GetTicks();

    SDL_Event e;
    e.type = ENGINE_INIT_GLOBALS;
    call_handlers(e);
    e.type = ENGINE_INIT;
    call_handlers(e);

    float total_ms_took = 0;
    const u32 perf_check_period_frames = 600;
    const float perf_ms_per_check = perf_check_period_frames * tick_period;
    int perf_checks_enabled = (config.enable_perf_checks == -1) ? 1 : config.enable_perf_checks;

    LOG_INFO("Server running. TPS: %d, perf checks: %s", target_tps, perf_checks_enabled ? "enabled" : "disabled");

    for (;;)
    {
        u32 frame_begin_tick = SDL_GetTicks();

        // Poll SDL events (quit, block updates, etc.)
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                call_handlers(e);
                goto server_exit;
                break;
            // case ENGINE_BLOCK_CREATE:
            // {
            //     // Block update event from layer
            //     block_update_event *upd_evt = (block_update_event *)&e;
            //     if (upd_evt->layer_ptr)
            //     {
            //         layer *l = (layer *)upd_evt->layer_ptr;
            //         push_block_update(&l->updates, upd_evt->x, upd_evt->y, upd_evt->new_id, l->block_size);
            //     }
            //     break;
            // }
            default:
                // Pass through to scripting/handlers
                call_handlers(e);
                break;
            }
        }

        // Tick all layers and execute logic
        if (latest_logic_tick + tick_period <= frame_begin_tick)
        {
            latest_logic_tick = SDL_GetTicks();
            e.type = ENGINE_TICK;
            call_handlers(e);

            // TODO: Broadcast accumulated updates from all layers at tick time (if any)
            // For now, this happens on-demand via Lua or when you explicitly call net_broadcast_update()
        }

        int loop_took = SDL_GetTicks() - frame_begin_tick;
        int chill_time = tick_period - loop_took;

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

server_exit:
    LOG_INFO("Server shutting down...");

    deinit_signal_handlers();
    deinit_backtrace();

    net_server_shutdown();
    scripting_close();

    return 0;
}
