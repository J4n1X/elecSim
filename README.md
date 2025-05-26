# Elecsim: A pulse simulation with the goal of creating logic circuits

Elecsim is a simulation written from scratch in C++, using Javidx9's [PixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) for rendering. 

It uses Bernard Teo's [Native File Dialog Extended](https://github.com/btzy/nativefiledialog-extended) for save/load dialogs. 

It simulates impulses running through a system, allowing for logic. In other words, it's an electronics simulation.

## Building the project

For the dependencies required to compile PixelGameEngine, please consult the [Wiki of PixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine/wiki). 

The dependencies required for NFDE [can be found here](https://github.com/btzy/nativefiledialog-extended?tab=readme-ov-file#linux)

Download or clone both of these projects into the "include" folder (which must be created).

The project itself can be compiled using cmake. On a command line, navigate into the project directory and use: 

```cmake . ```

To generate the required configuration for your system. To compile the project, all you must do after this is issue the following command: 

```cmake --build . --config <Debug|Release>```

The compiled binaries can be found in the newly created "bin" directory. 

## Using the program

The controls are as follows: 
  - 1-6: Select your tile. The order is: Wire, Junction, Emitter, Semiconductor, Button
  - Up, Down, Left, Right: Move the camera
  - J: Zoom in
  - K: Zoom out
  - F1: Open the console (use "help" for a list of available commands)
  - F2: Save file to disk
  - F3: Load file from disk
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

Inverter: Does what the name implies, and inverts a signal, outputting it in the direction of the facing.

### Examples
There are files in the "examples subdirectory to demonstrate the function of the different components. 

stresstest.grid: Attempts to draw a large amount of tiles (large for the time being at least, see "ToDos") 

tests.grid: Contains different small circuits which demonstrate the function of different tile types.

## ToDos
Sorted in order of how important I deem them to the proper function of the game.
  - Add editing tools: Area selection, copy, cut, paste, move around (DONE!)
  - Add a crossing Tile to allow for more flexible building.
  - Write more tests
  - Draw Sprites for the tiles instead of just drawing them out of primitives
  - Dispatch game logic to another thread so there's more time to render for more tiles
  - Implement compression for save files