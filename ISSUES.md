# Issues Found During Code Review

## Bugs

### 1. ~~Post-processing FBO logic inverted (P1)~~ — RESOLVED
Post-processing was removed (Phase 1 cut). See below.

## Removed Systems (Phase 1)

- Post-processing FBO path removed from `block_renderer_v2.c/.h` (post shader, FBO, `renderer_v2_begin_frame`/`end_frame`, `post.vert`/`post.frag`/`crt.frag`).
- Legacy `client_render()` removed from `rendering.c/.h`; main loop now renders rooms directly.
- `update_system.h` removed (never integrated into main loop).
- `multiblock_entity.c/.h` removed (layer_add_multiblock_entity was never implemented).
- `repeat_*` block registry handlers/fields/serialization removed (only used by legacy `blocks_old/`).
- `net.c`/`net.h` removed (no callers; `-lws2_32`/`-lWinmm` dropped from makefile).

## Architectural Issues

### 9. ~~Update system WIP (P1)~~ — RESOLVED (removed)

### 10. `layer_build_ground_physics()` O(n^2) per-block (P2)
`src/level.c:822-853` — creates one Box2D static body per block. Old commented-out version had greedy merging (quad grouping). Current approach is simpler but generates many Box2D bodies. Could be slow for large layers.

### 11. Monkey-patched Box2D body coords (P2)
`layer_build_ground_physics` computes position as `(f32)x * g_block_width - g_block_width * 0.5f + g_block_width`. The `- ... + g_block_width` pattern suggests coordinate system hack. Fragile.

### 12. `load_level_ack_registry` memory model unclear (P2)
Mixes heap-allocated level structs with stack-allocated `level` parameter in `lua_load_level`. Consistency issue.

## Style / Maintainability

### 13. Magic constants (P2)
- `tile_rand()` uses hardcoded arrays `funny_primes[10]`, `funny_shifts[10]`.
- Autotile tables hardcoded in `rendering.c`.
- `TABLE_SIZE 31` in hashtable — prime chosen for hash but undocumented.

### 14. ~~Commented-out code blocks (P3)~~ — RESOLVED
~200+ lines of dead/commented code removed (old ground physics in `level.c`, `init_script` remnants in `blockengine_base.c`, `SPECIAL_SIGNAL` handler in `scripting.c`, FBO clear, `lua_script_filename` handler, `block_entity_collision_script`, etc.).

### 15. ~~`vars.h` winsock include~~ — RESOLVED
`vars.h` never included winsock (false alarm). `-lws2_32`/`-lWinmm` (for net.c/enet, now removed) dropped from makefile.

## Performance

### 16. Instance buffer doubling (+ re-GL-buffer) per overflow (P2)
`block_renderer_v2.c:297-314` — on overflow, does realloc + full `glBufferData` with NULL. Growing 10k → 20k → 40k triggered rarely for large viewports, but `glBufferData` with NULL forces GPU discard + reallocation. Could use `glBufferData` with `GL_STREAM_DRAW` hint or orphan buffer pattern.

## Testing Gaps

### 17. No formal test framework
Testing is manual (run client/builder). TKV has fuzzing. No unit tests for vars, handle_table, spatial_grid, or rendering.

### 18. `tkv_value_get_root` exists but `tkv_value_to_tkv` may return NULL
Requires caller to check.

## Documentation

### 19. `todo.txt` contains only feature roadmap
No issue tracking for code bugs found here. This file (ISSUES.md) fills the gap.

## Outdated (exists in old files but no longer accurate)

### 20. README listed "client_app" target — renamed to "blockengine_base"
### 21. AGENTS.md claimed `make client_app` — actually `make blockengine_base`
