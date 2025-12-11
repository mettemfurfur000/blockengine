#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H 1

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Initialize signal handlers for critical errors (SIGSEGV, SIGABRT, etc.)
     * These handlers will log backtraces to a separate error log file
     * and attempt to exit gracefully.
     *
     * Supports both POSIX (Linux) and Windows (MinGW64) environments:
     * - Linux/Unix: Uses sigaction() for robust signal handling
     * - Windows: Uses signal() for basic exception handling
     *
     * Call this early in main() after log_start()
     */
    void init_signal_handlers(void);

    /**
     * Deinitialize signal handlers
     * Call this before exiting the program
     */
    void deinit_signal_handlers(void);

#ifdef __cplusplus
}
#endif

#endif
