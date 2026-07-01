# blockengine

A lightweight 2D block-based game engine. C + C++, SDL2 windowing, OpenGL rendering, Lua scripting. Focus on efficient block rendering, scriptable behaviors, and level editing.

## Architecture

### Level
```
level → rooms[] → layers[] → blocks[]
```
- **level**: top-level container. Holds rooms + registries.
- **room**: game world area. Fixed width/height. Owns Box2D world for physics.
- **layer**: grid of blocks. Each block stores ID (1/2/4/8 bytes) + optional var handle (4 bytes). Separate spatial grid per layer for culling.
- **block**: 64-bit ID into block_registry. ID 0 = void/air.

Layers support flags: `USE_VARS`, `HAS_REGISTRY`, `HAS_ENTITIES`, `STATIC`.

### Block Registry
`registries/<name>/blocks/` holds `.blk` files defining each block type. Each file parsed as key-value properties. Handlers validate dependencies, slots, and incompatibilities.

Properties system supports: id, texture, sounds, vars, fps, frame/type/flip/rotation controllers, autotile (3 types: 3x3, 4x4, 47-tile), interpolation, scripts, inputs.

Registry compiled to binary `.brg` format for fast loading. Embedded lua bytecode support.

### Rendering Pipeline
```
layer_slice[] → renderer_v2_begin_frame → render_layer (per slice) → renderer_v2_end_frame
```
- **Instanced rendering**: single draw call per layer via `glDrawElementsInstanced`. Instance buffer starts at 10k capacity, doubles on overflow.
- **Spatial grid**: 16x16 cells. Only non-empty cells in viewport iterated. Autotile frames cached per block.
- **Post-processing**: FBO + shader pipeline (commented out/partial — see issues).
- **Entities**: block_entity objects rendered via Box2D body position, with linear interpolation.

### Lua Scripting
`blockengine.register_handler(event_id, func)` — event-driven. Events:
- SDL input events (keyboard, mouse, joystick, etc.)
- Engine events: BLOCK_UPDATE, BLOCK_ERASE, BLOCK_CREATE, BLOB_UPDATE/ERASE/CREATE
- TICK (per-tick logic), INIT, INIT_GLOBALS, FRAME_PRE, FRAME_POST

Exposed objects: Level, Room, Layer, BlockRegistry, BlockEntity, Sound, VarHandle, Image.

Lua scripts can be compiled to bytecode (with module bundling via `MODL` container format).

### Variable System (Vars)
Blocks store typed variables as compact binary blobs (blob format: `[letter:1][size:1][value:size]`). Fast O(1) lookup via pre-computed offset table.

Controllers cast vars to rendering: animation frame, type, flip, rotation, position offsets, interpolation.

### Physics
Box2D integration. Each room can host a physics world. Layers can generate static ground collision. Block entities get Box2D bodies.

### Audio
SDL2_mixer. Sound chunks loaded per block resource. Script-triggered playback.

### TKV Format
Typed key-value tree format for serialization. Strong typing (bool, i8-u64, f32-f64, str, arr, vec3, quat, nested tkv). 6-bit compressed keys. Binary format with O(1) access. Endian-aware stream I/O. Used for config, networking, data exchange.

### Update System
Accumulator-based block/variable change tracking. Records block ID changes and var component updates. Supports replay on remote layers (networking).

### Entity System
- **block_entity**: single block with Box2D body, vars, scale, multiblock shapes.
- **multiblock_shape**: grid of block IDs forming shapes (T-block, L-block, etc.) with per-block physics overrides.
- Handle-table based allocation.

### Networking
Cross-platform TCP + UDP. Length-prefixed message protocol. Planned integration with TKV for sync data. WIP (see todo.txt).

### Signal Handling
Custom SIGSEGV/SIGABRT handler → log backtrace to file + graceful exit. Uses libbacktrace on both Linux and Windows (MinGW).

### Memory
- `arena.h`: bump allocator for scratch/temporary allocations.
- `handle.h`: handle table (u16 index + 15-bit validation + 1-bit active). Opaque references to objects.
- `blob`: length-prefixed byte buffer.
- `vec.h`: dynamic array library (from libs/vec).

## Build

**Dependencies**: SDL2, SDL2_image, SDL2_ttf, SDL2_mixer, Lua 5.4, Box2D, epoxy (OpenGL loader), libbacktrace, zlib.

```bash
make              # Build all targets
make blockengine_base  # Game client
make builder      # Registry compiler/saver
make tex_gen      # Texture atlas generator
make tkv_test     # TKV format test
make clean
```

Windows (MSYS2): `pacman -S mingw-w64-x86_64-toolchain make mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-lua`
Linux: `sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev liblua5.4-dev libbox2d-dev`

## Project Structure
```
blockengine/
├── include/          # Headers (.h)
├── src/
│   ├── basic/        # Core: arena, backtrace, handle, hashtable, logging, spatial_grid, timer, vars, vec_math
│   ├── graphics/     # Rendering: block_renderer_v2, opengl, sdl2, atlas_builder, image_editing
│   ├── scripting/    # Lua: bindings per type (level, entity, registry, sound, image, render_rules)
│   ├── tkv/          # TKV format implementation
│   └── language/     # Tokenizer
├── libs/             # External: dirent, stb, vec
├── mains/            # Entry points: blockengine_base (client), builder (registry compiler), tex_gen, tkv_test
├── instance/         # Game assets: levels/, registries/<name>/{blocks/, textures/, scripts/, sounds/}, shaders/
└── build/            # Output binaries
```

## Running
```bash
./build/blockengine_base [options]
  -r, --registry <path>  Registry folder name (default: engine)
  -w, --width <px>       Screen width
  -h, --height <px>      Screen height
  -f, --fps <rate>       Target FPS (default: 60)
  -t, --tps <rate>       Tick rate (default: 20)
  -F / -W                Fullscreen / Windowed
  -p / -P                Enable/disable perf checks
```

## Profiling
```bash
make PERF=1
./build/blockengine_base
gprof build/blockengine_base gmon.out > analysis.txt
```
