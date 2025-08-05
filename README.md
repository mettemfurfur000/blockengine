# blockengine

A lightweight 2D block-based engine focusing on efficient rendering and scriptable block behavior.

## Core Features

### World Structure
- Grid-based 2D world composed of blocks
- Multi-layered world system with independent layers
- Room-based world organization for efficient memory management
- Flexible block properties system with variable storage

### Graphics Engine
- Hardware-accelerated rendering using OpenGL
- Efficient instanced rendering for blocks
- Texture atlas support for optimized memory usage
- Rich block visualization features:
  - Animation support with configurable FPS
  - Rotation support (0-360 degrees)
  - Texture flipping (horizontal/vertical)
  - Scale and stretching capabilities
  - Location-based pseudo-random patterns
  - Frame and type variations per block

### Block System
- Comprehensive block registry system
- Robust resource management (loading/saving)
- Variable-based block properties
- Advanced animation controls:
  - Frame-based animations
  - Time-based animations (FPS control)
  - Random frame selection
  - Frame override capability

### Scripting System
- Lua integration for block behavior
- Extensive event system including:
  - Block lifecycle events (set, erase, move)
  - Data management events (create, remove)
  - Custom signal handling
- Direct access to rendering parameters
- World manipulation capabilities
- Sound system integration

### Audio System
- Sound loading and playback support
- Script-controlled audio playback
- Integration with SDL2_mixer

In summary, developers can create a layer of game logic (marked "d", for example) and build a "mechanism" from the blocks that accepts user input and moves the player around on the game layer, as well as create other mechanisms for monsters, interfaces, and more.

## Technical Details

### Rendering System
- OpenGL-based rendering pipeline
- Efficient instanced rendering with attribute system:
  - Position (float x, y)
  - Scale (float scale_x, scale_y)
  - Rotation (float, radians)
  - Frame/Type indices (uint8)
  - Flags for effects (uint8)
- Post-processing support
- Frame buffer abstraction

### Block Properties
- Variable-based block data storage
- Controllers for:
  - Animation frames
  - Block types/variations
  - Rotation
  - Flip states
- Frame override capability
- Random positioning system

### Room System
- Replaced chunk system for better memory management
- Dynamic room loading/unloading
- Efficient block access and modification

## Potential Future Features
- Particle system using block instances
- Connected textures for blocks
- Lighting system
- Basic entity system (if needed)

## Dependencies
- OpenGL (via epoxy)
- SDL2 and SDL2_mixer
- Lua for scripting

## Note
The engine is designed to be lightweight and efficient, focusing on 2D block-based games and simulations. While it could support entities and physics through Box2D integration, the current focus is on block-based mechanics and efficient rendering.
