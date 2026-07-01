# AGENTS.md — Blockengine Developer Guide

Project: lightweight 2D block game engine. C + C++, SDL2, OpenGL, Lua.

## Build

```bash
make                    # All targets
make blockengine_base   # Game client
make builder            # Registry compiler
make tex_gen            # Texture atlas generator
make tkv_test           # TKV format test
make clean
make PERF=1             # Profiling build
```

Deps (MSYS2): `mingw-w64-x86_64-toolchain make mingw-w64-x86_64-SDL2{,_image,_ttf,_mixer} mingw-w64-x86_64-lua libbacktrace`
Deps (Linux): `build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev liblua5.4-dev libbox2d-dev libepoxy-dev`

Windows-only flags: `-lws2_32 -lWinmm -lbacktrace -lopengl32 -lepoxy.dll`
Linux-only flags: `-llua5.4 -lepoxy`

## Code Style

- `.clang-format`: Microsoft style, 4-space indent, pointer right, braces wrap.
- `snake_case` for everything. `UPPER_CASE` for macros/constants.
- Types from `include/general.h`: `u8/u16/u32/u64`, `i8/i16/i32/i64`, `f32/f64`.
- Return `SUCCESS` (0) or `FAIL` (-1). Use `assert()` for invariants (custom backtrace on fail).
- Logging: `LOG_MESSAGE/ERROR/WARNING/INFO/DEBUG(level 1-5)`. Set `LOG_LEVEL` in `include/logging.h`.
- `SAFE_FREE(ptr)` macro frees + nullifies.
- `extern "C"` guards for C++ interop.
- No comments unless complex non-obvious logic.

## Key Files

| File | Role |
|------|------|
| `include/level.h` | Level/room/layer structs, block ops |
| `include/block_registry.h` | Block resource loading, storage |
| `include/block_renderer_v2.h` | Instanced OpenGL renderer |
| `include/rendering.h` | layer_slice, client_render |
| `include/events.h` | Engine event system |
| `include/spatial_grid.h` | Spatial partitioning + autotile cache |
| `include/vars.h` | Block variable blob system |
| `include/handle.h` | Opaque handle table |
| `include/tkv.h` | TKV serialization format |
| `include/scripting.h` | Lua state management |
| `include/data_io.h` | Endian-aware stream I/O (plain/gzip/buffer) |
| `mains/blockengine_base.c` | Client entry + main loop |

## Adding New Source Files

Create `.c` in `src/<module>/`, `.h` in `include/`. Makefile auto-discovers via `find` glob.

## Adding Lua Bindings

1. Declare functions in `include/scripting/<type>.h`
2. Implement in `src/scripting/lua_<type>.c`
3. Register in `lua_register_engine_objects()` in `scripting_bindings.c`

## Dependencies Map

- **SDL2**: window, input, events
- **epoxy (OpenGL loader)**: GL functions
- **Box2D**: physics
- **Lua 5.4**: scripting
- **SDL2_mixer**: audio
- **zlib**: gzip I/O streams
- **libbacktrace**: crash backtraces
- **libs/vec**: dynamic arrays
- **libs/stb**: image I/O
- **libs/dirent**: directory iteration (Windows)

## Common Pitfalls

- Block ID 0 = void. Registry auto-fills gaps with filler entries.
- `total_bytes_per_block` = `block_size + sizeof(handle32)` (4 bytes for handle).
- Layer var_pool uses handle_table for blob storage.
- Spatial grid invalidates on `block_set_id()`. Autotile cache also invalidated on neighbor change.
- Builder is headless (no graphics). Client connects to display.
- Post-processing currently disabled (FBO logic has issues).
- `sizeof(handlers)` in `scripting.c:586` is wrong (byte count vs element count) — but bounds never reached in practice.
