#ifndef BACKTRACE_ENGINE_H
#define BACKTRACE_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct backtrace_state;

    // Exported for signal handlers
    extern struct backtrace_state *bt_state;

    void init_backtrace(void);
    void deinit_backtrace(void);
    void print_backtrace(void);

#ifdef __cplusplus
}
#endif

#endif