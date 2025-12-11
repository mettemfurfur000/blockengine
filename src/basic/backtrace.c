#include "include/backtrace.h"

#include <backtrace-supported.h>
#include <backtrace.h>

#include "include/logging.h"

static struct backtrace_state *bt_state = NULL;

// Callback for backtrace_full to print stack frames
static int backtrace_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function)
{
    (void)data; // Unused

    if (filename == NULL)
        filename = "???";
    if (function == NULL)
        function = "???";

    LOG_ERROR("  [0x%lx] %s:%d in %s", (unsigned long)pc, filename, lineno, function);

    return 0;
}

// Error callback for backtrace_full
static void engine_backtrace_error_callback(void *data, const char *msg, int errnum)
{
    (void)data;   // Unused
    (void)errnum; // Unused

    LOG_ERROR("  Backtrace error: %s", msg);
}

void init_backtrace(void)
{
    bt_state = backtrace_create_state(NULL, 1, engine_backtrace_error_callback, NULL);

    if (!bt_state)
    {
        LOG_WARNING("Failed to initialize backtrace state");
    }
    else
    {
        LOG_DEBUG("Backtrace system initialized");
    }
}

void deinit_backtrace(void)
{
    // libbacktrace doesn't require explicit deinitialization in most cases
    // but we can set the state to NULL for safety
    bt_state = NULL;
    LOG_DEBUG("Backtrace system deinitialized");
}

void print_backtrace(void)
{
    if (!bt_state)
    {
        LOG_ERROR("Backtrace system not initialized");
        return;
    }

    LOG_ERROR("=== BACKTRACE START ===");
    backtrace_full(bt_state, 1, backtrace_callback, engine_backtrace_error_callback, NULL);
    LOG_ERROR("=== BACKTRACE END ===");
}