# blockengine

This project is for fun and aims to create a little engine with the following main ideas:

## About the Data
- Everything consists of blocks on a two-dimensional square grid.
- Creatures in the world are bound to the grid, although this may change in the future for better game play.
- The world is composed of two-dimensional layers, each marked with one letter (symbol), and has their own constant dimensions (functions can be written to change the dimensions).
- Each layer is divided into chunks which are square and have their own size.
- Each chunk contains the blocks themselves.

## World Behavior
- Block behavior is determined either by Lua scripts or C functions (for better performance).
- The world is updated ten times per second. Each block in a loaded chunk runs its own update function, if it has one.
- Only loaded chunks are updated. Inactive chunks (for example, those without updates for 100 ticks) are unloaded to disk.
- If a chunk attempts to access a block from an unloaded chunk during a chunk update, it is loaded and marked as active.
- A given block can access any block in the current world and can do anything with it.

## Graphics and More
- Blocks can interact with SDL2 (you would have to register functions for Lua), allowing for control over what the player sees and hears.
- Blocks can decide which textures to use to render themselves on the screen.

In summary, developers can create a layer of game logic (marked "d", for example) and build a "mechanism" from the blocks that accepts user input and moves the player around on the game layer, as well as create other mechanisms for monsters, interfaces, and more.

## Roadmap

- [x] Basic operations on blocks
- [x] Block memory control
- [x] Chunk memory control
- [x] Saving and Loading 
- [x] Tests for everything above
- [ ] Basic game cycle
   - [ ] Loading or generating a world
   - [ ] Inactive chunks unloading from memory
   - [ ] World update and update rate setting
   - [ ] Test for everything above
- [x] Block Registry
   - [x] Saving and loading
   - [x] Basic managment of duplicated/missing resources
- [ ] Lua scripting for blocks
   - [x] Basic passive scripts
   - [x] Functions for simple operations on blocks
   - [x] Block registry block querry
   - [ ] Give lua access to rendering rules (camera pos, etc)
   - [ ] Allow lua to create or delete world layers
   - [ ] Allow lua to load/unload chunks, forced or on-demand
   - [x] Custom events for block interactions
      - [x] on_set, on_erase, on_move, 
      - [ ] on_load, on_unload, on_data_create, on_data_remove, on_signal
- [ ] Graphics
   - [x] Basic static textures
   - [x] Animated textures
      - [x] FPS setting
   - [x] Data-driven texture selection for individual blocks
   - [x] Animation-compatible data-driven textures
   - [ ] Particles
   - [x] Rotatable block textures, for turrets/cogs/valves
   - [x] Flippable textures for some memory economy
   - [x] Pseudorandom texture patterns based on block location
   - [ ] Connectable textures
   - [ ] Lightning?
- [ ] Sound
   - [ ] Load and play sounds
   - [ ] Allow scripts to play them

That's the list I'm currently aiming for. Hope this helps!
