#include "include/signal_handler.h"
#include "include/backtrace.h"
#include "include/logging.h"

#include <backtrace.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static FILE *error_log_file = NULL;
static const char *ERROR_LOG_FILENAME = "crash_log.txt";

/**
 * Backtrace callback for signal handler context
 */
static int signal_backtrace_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function)
{
    (void)data; // Unused
    FILE *f = (FILE *)data;

    if (filename == NULL)
        filename = "???";
    if (function == NULL)
        function = "???";

    if (f)
    {
        fprintf(f, "  [0x%lx] %s:%d in %s\n", (unsigned long)pc, filename, lineno, function);
        fflush(f);
    }

    return 0;
}

/**
 * Error callback for backtrace in signal handler
 */
static void signal_backtrace_error_callback(void *data, const char *msg, int errnum)
{
    (void)errnum; // Unused
    FILE *f = (FILE *)data;

    if (f)
    {
        fprintf(f, "  Backtrace error: %s\n", msg);
        fflush(f);
    }
}

/**
 * Print to both the main log and the error log
 */
static void log_to_error_file(const char *format, ...)
{
    va_list args;

    // Print to main log file
    if (log_enabled && log_file)
    {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }

    // Print to error log file
    if (error_log_file)
    {
        va_start(args, format);
        vfprintf(error_log_file, format, args);
        va_end(args);
        fprintf(error_log_file, "\n");
        fflush(error_log_file);
    }

    // Always print to stderr
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

/**
 * Signal handler for critical errors
 */
static void critical_signal_handler(int sig)
{
    const char *sig_name = "UNKNOWN";

    switch (sig)
    {
    case SIGSEGV:
        sig_name = "SIGSEGV (Segmentation Fault)";
        break;
    case SIGABRT:
        sig_name = "SIGABRT (Abort)";
        break;
    case SIGFPE:
        sig_name = "SIGFPE (Floating Point Exception)";
        break;
    case SIGILL:
        sig_name = "SIGILL (Illegal Instruction)";
        break;
        // case SIGBUS:
        //     sig_name = "SIGBUS (Bus Error)";
        //     break;
    }

    // Get current timestamp
    time_t now = time(NULL);
    char *timestr = ctime(&now);
    timestr[strlen(timestr) - 1] = '\0';

    log_to_error_file("\n");
    log_to_error_file("==============================================");
    log_to_error_file("[%s] CRITICAL ERROR - Signal caught: %s", timestr, sig_name);
    log_to_error_file("==============================================");

    // Print backtrace
    if (bt_state != NULL)
    {
        log_to_error_file("=== BACKTRACE START ===");

        backtrace_full(bt_state, 1, signal_backtrace_callback, signal_backtrace_error_callback, error_log_file);

        log_to_error_file("=== BACKTRACE END ===");
    }
    else
    {
        log_to_error_file("Backtrace not initialized!");
    }

    log_to_error_file("==============================================");
    log_to_error_file("Crash log written to: %s", ERROR_LOG_FILENAME);
    log_to_error_file("==============================================\n");

    // Attempt to close files gracefully
    if (error_log_file)
    {
        fclose(error_log_file);
        error_log_file = NULL;
    }

    log_end();

    // Exit with error code
    exit(128 + sig);
}

void init_signal_handlers(void)
{
    // Open error log file
    error_log_file = fopen(ERROR_LOG_FILENAME, "a");
    if (!error_log_file)
    {
        LOG_WARNING("Failed to open crash log file: %s", ERROR_LOG_FILENAME);
    }
    else
    {
        LOG_DEBUG("Crash log file opened: %s", ERROR_LOG_FILENAME);
    }

#ifdef _WIN32
    // Windows: Use signal() for basic exception handling
    // Note: Windows signal handling is more limited than POSIX
    signal(SIGABRT, critical_signal_handler);
    signal(SIGFPE, critical_signal_handler);
    signal(SIGILL, critical_signal_handler);
    signal(SIGSEGV, critical_signal_handler);
    LOG_DEBUG("Signal handlers initialized (Windows mode)");
#else
    // Linux/Unix: Use sigaction for more robust handling
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = critical_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
#ifdef SIGBUS
    sigaction(SIGBUS, &sa, NULL);
#endif

    LOG_DEBUG("Signal handlers initialized (POSIX mode)");
#endif
}

void deinit_signal_handlers(void)
{
    if (error_log_file)
    {
        fclose(error_log_file);
        error_log_file = NULL;
    }

    LOG_DEBUG("Signal handlers deinitialized");
}
