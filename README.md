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
