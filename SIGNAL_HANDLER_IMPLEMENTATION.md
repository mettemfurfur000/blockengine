# Signal Handler Implementation for Crash Logging

## Overview
Added comprehensive signal handling to catch critical errors (segmentation faults, abort signals, etc.) and automatically generate detailed crash logs with backtraces.

## Changes Made

### New Files
1. **`include/signal_handler.h`** - Header for signal handling initialization
   - `init_signal_handlers()` - Initialize signal handlers (call after `log_start()`)
   - `deinit_signal_handlers()` - Clean up signal handlers (call before exit)

2. **`src/basic/signal_handler.c`** - Signal handler implementation
   - Catches SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS
   - Generates detailed crash logs with backtrace information
   - Logs to separate `crash_log.txt` file for critical errors
   - Also logs to main log file and stderr for visibility

### Modified Files

1. **`include/backtrace.h`**
   - Exported `bt_state` variable so signal handlers can access it
   - Added `#include <stdint.h>` for `uintptr_t` type

2. **`src/basic/backtrace.c`**
   - Changed `bt_state` from `static` to global so signal handlers can use it

3. **`mains/client.c`**
   - Added `#include "include/signal_handler.h"`
   - Added `init_signal_handlers()` call in `main()` after `init_backtrace()`
   - Added `deinit_signal_handlers()` call before exit

## Behavior

When a critical signal is caught (e.g., segmentation fault):

1. **Immediate Logging**: Signal is caught and logged with timestamp
2. **Backtrace Generation**: Full backtrace is captured using libbacktrace
3. **Multi-destination Output**:
   - Main log file (e.g., `client.log`)
   - Separate crash log file (`crash_log.txt`)
   - stderr (for console visibility)
4. **Graceful Exit**: Process exits with code `128 + signal_number`

## Crash Log Format

```
==============================================
[timestamp] CRITICAL ERROR - Signal caught: SIGSEGV (Segmentation Fault)
==============================================
=== BACKTRACE START ===
  [0x...] file.c:123 in function_name
  [0x...] other.c:456 in other_function
  ...
=== BACKTRACE END ===
==============================================
Crash log written to: crash_log.txt
==============================================
```

## Usage

The signal handlers are automatically initialized when `init_signal_handlers()` is called in `main()`. No additional action needed - if a segfault occurs, you'll get:
- A detailed backtrace in the crash log
- Preservation of the regular log for context
- Clear identification of the error location

## Notes

- Signal handlers work across all platforms where POSIX signals are available
- The crash log is opened in append mode, so multiple crashes are recorded
- All file operations use flushing to ensure data is written immediately
