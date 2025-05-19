# Elecsim: A pulse simulation with the goal of creating logic circuits

Elecsim is a simulation written from scratch in C++, using Javidx9's [PixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) for rendering. 

It simulates pulses travelling through a system of tiles, which can process these pulses in different manners. 

## Building the project

For the dependencies required to compile PixelGameEngine, please consult the [Wiki of PixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine/wiki). 

The project itself can be compiled using cmake. On a command line, navigate into the project directory and use: 

```cmake . ```

To generate the required configuration for your system. To compile the project, all you must do after this is issue the following command: 

```cmake --build . --config <Debug|Release>```

The compiled binaries can be found in the newly created "bin" directory. 

## Using the program

The controls are as follows: 
  - 1-5: Select your tile. The order is: Wire, Junction, Emitter, Semiconductor, Button
  - Up, Down, Left, Right: Move the camera
  - J: Zoom in
  - K: Zoom out
  - S: Save to "grid.bin"
  - L: Load from "grid.bin"
  - Comma: Speed up the update rate
  - Period: Slow down the update rate
  - R: Change the facing of the tile you are about to place
  - Space: Pause the simulation
  - Left Click (while the simulation is paused): Place a tile
  - Middle click (while the simulation is paused): Erase a tile
  - Right click (while the simulation is running): Interact with certain tiles (Button, Semiconductor and Emitter)
  - Escape: Close the game

When you pause the simulation, the state is kept until you place a new tile or you remove one. Upon either of these events, the simulation is entirely reset.

### Tile behavior

Wire: Take inputs from any side but the one it is facing, and forward them in the direction it is facing.

Junction: Take inputs from any side, and spread it in all directions excluding the one where it came from.

Emitter: While active, keep pulsing (1 pulse is sent out every 2 ticks)

Semiconductor: When activated from the side, it gets primed. Then, an activation from the bottom will cause it to emit a pulse in the direction it's facing.

Button: Does nothing, unless interacted with, in which case it will pulse once.
