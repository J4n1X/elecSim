# Elecsim: A pulse simulation with the goal of creating logic circuits

Elecsim is a tile-based logic simulation, and intended to be game, not an actual electronics simulator. It is written in C++ and is cross-platform compatible.

Below, you can see some of the things you can build with Elecsim.

![image 8-bit Adder, AND-Gate and XOR-Gate implemented in Elecsim](media/images/componentGallery.png)

## Building the project

For the dependencies required to compile PixelGameEngine, please consult the [Wiki of PixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine/wiki). 

The dependencies required for NFDE [can be found here](https://github.com/btzy/nativefiledialog-extended?tab=readme-ov-file#linux)

Download or clone both of these projects into the "include" folder (which must be created).

The project itself can be compiled using cmake. On a command line, navigate into the project directory and use: 

```
mkdir build

cd build

cmake ..
```

If you'd like to use the the old way of processing tile updates, pass along ```-DSIM_CACHING=OFF``` after the initial configuration has completed.

To generate the required configuration for your system. To compile the project, all you must do after this is issue the following command: 

```cmake --build .```

The compiled binaries can be found in the newly created "bin" directory. 

## Using the program

The controls are as follows: 
  - 1-7: Select your tile. The order is: Wire, Junction, Emitter, Semiconductor, Button, Inverter, Crossing
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
  - Left Mouse Button (while the simulation is paused): Place a tile
  - Left Mouse Button (while the simulation is running): Interact with certain tiles (Button, Semiconductor and Emitter)
  - Right Mouse Button (while the simulation is paused): Erase a tile
  - Middle Mouse Button: Pan the camera around.
  - Escape: Close the game

When you pause the simulation, the state is kept until you place a new tile or you remove one. Upon either of these events, the simulation is entirely reset.

### Tile behavior

Wire: Take inputs from any side but the one it is facing, and forward them in the direction it is facing.

Junction: Take inputs from any side, and spread it in all directions excluding the one where it came from.

Emitter: While active, keep pulsing. It changes states every tick.

Semiconductor: When activated from the side, it gets primed. Then, an activation from the bottom will cause it to emit a pulse in the direction it's facing.

Button: Does nothing, unless interacted with, in which case it will pulse once.

Inverter: Does what the name implies, and inverts a signal, outputting it in the direction of the facing.

Crossing: Takes one input from one side, and puts it out the other. This allows wires to cross. Get messy!

### Examples
There are files in the "examples subdirectory to demonstrate the function of the different components. 

stresstest.grid: Attempts to draw a large amount of tiles. (Does not stresstest simulation speeds)

tests.grid: Contains different small circuits which demonstrate the function of different tile types.

componentGallery.grid: Contains multiple more complex circuits, such as an 8-bit adder, XOR and AND-Gate. (See the image above.)

## ToDos
Sorted in order of how important I deem them to the proper function of the game.
  - Add editing tools: Area selection, copy, cut, paste, move around (DONE!)
  - Add a crossing tile to allow for more flexible building. (DONE!)
  - Implement Signal Path Preprocessing (DONE!)
  - Write more tests (DONE!)
  - Draw Sprites for the tiles instead of just drawing them out of primitives
  - Dispatch game logic to another thread so there's more time to render for more tiles
  - Implement compression for save files


# Credits

Massive thanks go out to these outstanding projects! Without them, this would have been a whole lot harder to actually get done.

  - [Native File Dialog Extended](https://github.com/btzy/nativefiledialog-extended) by Bernard Teo
  - [PixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) by Javidx9