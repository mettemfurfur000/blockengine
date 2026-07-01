# Issues Found During Code Review

## Bugs

### 1. Post-processing FBO logic inverted (P1)
`src/graphics/block_renderer_v2.c:354-376`
`renderer_v2_begin_frame` is no-op (FBO bind commented out). `renderer_v2_end_frame` binds FBO 0, draws post shader sampling post_texture, THEN binds post_fbo. Order should be: begin_frame → bind post_fbo + clear, end_frame → bind FBO 0 + draw post shader. Currently renders stale/empty texture.

## Architectural Issues

### 9. Update system WIP (P1)
`include/update_system.h` — designed for multiplayer block/var sync but large commented-out code blocks. `update_acc_new`, `update_block_push`, etc. defined but not integrated into main loop. Network sync incomplete.

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

### 14. Commented-out code blocks (P3)
~200+ lines of dead code across:
- `level.c:723-820` (old ground physics impl)
- `blockengine_base.c:197` (init script)
- `scripting.c:497` (SPECIAL_SIGNAL handler)
- `block_renderer_v2.c:356-358` (FBO clear)

### 15. `vars.h` winsock include (P3)
`include/vars.h:4-8` — includes `<winsock.h>` on Win64 for byte-order functions but project has `include/endianless.h`. Dead include.

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
