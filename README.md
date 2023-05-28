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
- [ ] List of permanent block parameters
- [ ] Saving and loading permanent block parameters
- [ ] Lua scripting for blocks
- [ ] Graphics

That's the list I'm currently aiming for. Hope this helps!
