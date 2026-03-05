# AGENTS.md - Blockengine Developer Guide

This file provides guidelines for agents working on the blockengine codebase.

## Project Overview

Blockengine is a lightweight 2D block-based game engine written in C with some C++. It uses SDL2 for windowing/input, OpenGL for rendering, and Lua for scripting. The project builds on Windows (MSYS2/MinGW) and Linux.

## Build Commands

### Building the Project

```bash
make              # Build all targets (client_app, builder)
make client_app   # Build the game client
make builder      # Build the level editor
make tex_gen      # Build the texture atlas generator
make clean        # Clean build artifacts
```

### Building with clangd Support

```bash
# Option 1: Using get_compile_commands.sh (rebuilds everything)
./get_compile_commands.sh

# Option 2: Manual setup with compiledb
python3 -m venv .compiledbenv
source .compiledbenv/bin/activate
pip install compiledb
make VERBOSE=1 -j 8 -B > ./build_log.txt
compiledb < build_log.txt
```

### Testing

- There is **no formal test framework** - testing is done manually by running the client or builder
- Run `make test` on Linux to build and test

### Dependencies

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-toolchain make mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-lua
```

**Linux:**
```bash
sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev liblua5.3-dev
```

## Code Style Guidelines

### Formatting

- **Follow `.clang-format`** - Uses Microsoft style with these settings:
  - Indent width: 4 spaces
  - Pointer alignment: Right
  - Braces wrap after control statements, functions, classes, structs, enums
  - Pack constructor initializers: Never

- Run `clang-format -i <file>` to format code

### Naming Conventions

- **Variables**: lowercase with underscores (snake_case) - e.g., `block_count`, `handle_table`
- **Structs/Enums**: lowercase with underscores - e.g., `struct handle_table_slot`
- **Functions**: lowercase with underscores - e.g., `handle_table_create()`
- **Macros/Constants**: UPPERCASE - e.g., `MAX_PATH_LENGTH`, `SAFE_FREE`
- **Types**: Use prefixed types (see below)

### Type Definitions

Use fixed-width integer types from `include/general.h`:

```c
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
```

Also use `bool` from `<stdbool.h>`.

### Include Guidelines

- Project headers: `#include "include/xxx.h"` (quotes, not angle brackets)
- External headers: `#include <xxx.h>` (angle brackets)
- Order: Project headers first, then standard library, then external libraries
- Example:
```c
#include "include/logging.h"
#include "include/general.h"
#include <stdlib.h>
#include <string.h>
#include <lua.h>
```

### Logging

Use the logging macros defined in `include/logging.h`:

```c
LOG_MESSAGE(format, ...)  // Level 1
LOG_ERROR(format, ...)     // Level 2
LOG_WARNING(format, ...)   // Level 3
LOG_INFO(format, ...)      // Level 4
LOG_DEBUG(format, ...)     // Level 5
```

Set `LOG_LEVEL` in the header (2-3 for release, 5 for debug).

### Error Handling

- Return `SUCCESS` (0) or `FAIL` (-1) for functions
- Use `assert()` for internal invariants (has custom backtrace in debug builds)
- Check allocations: `if (!ptr) return FAIL;` or `if (!ptr) return NULL;`
- Use the `SAFE_FREE(ptr)` macro to safely free memory

### Memory Management

```c
// Allocations
void *ptr = malloc(size);
if (!ptr) return FAIL;

// Or with calloc
void *ptr = calloc(count, size);
if (!ptr) { free(original); return FAIL; }

// Freeing
SAFE_FREE(ptr);
```

### C/C++ Interop

- Headers must use `extern "C"` guards for C++ compatibility:
```c
#ifdef __cplusplus
extern "C" {
#endif

// declarations

#ifdef __cplusplus
}
#endif
```

### Comments

- **DO NOT add comments** unless explaining complex non-obvious logic
- Keep code self-documenting with clear naming

### Best Practices

1. Always check function return values for error conditions
2. Validate input parameters at function entry
3. Use handle tables (`include/handle.h`) for opaque references to objects
4. Put related functions in the same source file with matching header
5. Keep functions focused and under ~100 lines when possible
6. Use static for internal linkage functions/variables

## Project Structure

```
blockengine/
├── include/          # Public headers (.h)
├── src/              # Implementation (.c, .cpp)
│   ├── basic/        # Core utilities (logging, handles, vars, etc.)
│   ├── graphics/     # Rendering, OpenGL, SDL2 utilities
│   ├── scripting/    # Lua bindings
│   └── network/      # Networking (ENet)
├── libs/             # External libraries (dirent, enet, stb, vec)
├── mains/            # Entry points (client.c, builder.c, tex_gen.c)
├── instance/         # Game assets (textures, scripts, levels)
├── build/            # Output binaries
├── obj/              # Object files
└── makefile          # Build system
```

## Common Development Tasks

### Adding a New Source File

1. Create `src/<module>/filename.c`
2. Add corresponding `include/filename.h` with declarations
3. Include guards: `#ifndef FILENAME_H` / `#define FILENAME_H 1`
4. Add to makefile if needed (auto-detected via glob)

### Adding a New Lua Binding

1. Create static binding functions in `src/scripting/scripting_bindings.c`
2. Register in appropriate `lua_xxx_register()` function
3. Add function declarations to `include/scripting_bindings.h`

### Debugging

- Enable debug logging: Set `#define LOG_LEVEL 5` in `include/logging.h`
- Use `LOG_DEBUG()` for verbose output
- Backtraces are printed on assert failures (debug builds)
