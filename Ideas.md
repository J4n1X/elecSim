# Simulation and Optimization Ideas

## Tile Lookup Optimization
There is a much, much faster way to address the tiles when simulating. Basically, when the time comes to simulate for the first time after a reset, we'll build an index on a massive field of tiles, most of which will be empty, sure, but we can then do index calculations to find tiles. That'll solve the problem with the tile lookup, but not with the SignalEdge lookup. I don't think there's a fast solution for it. I am considering of thinking of a new way with which we could solve the problem of loop detection. I believe that a thing that could help massively with performance is detecting different non-conditional paths a signal can take. That's for the far future, though.

## Idea: Subcircuits
Might have to add a couple tiles for that, inputs and outputs, and those things will be the first item to draw over multiple tile fields. While we are at it, at the point of saving, it would be awesome if you could select an option to preprocess the states, which would speed up the simulation when you're not looking inside the subcircuit. Yes, you should be able to look into the subcircuit at runtime, clicking on it would basically open a little PIP window. Far future stuff, I know.

## Idea: "Just In Time" Simulation Processing
This would hold a list of changes to be applied when a certain circuit state is reached. If this state has not been reached ever before, it'd be processed first, and then applied. This would require GridTiles to specify that their output signal is conditional (Needs more than one signal to trigger or is able to be interacted with). When a conditional tile is hit, then the JIT processor would be called, and it would process all the updates until another conditional tile is hit, at which point, it would just stop.

### Additional idea: JIT preprocessing
Why not preprocess every possible conditional tile's changes? That will take a lot of time when starting the simulation, but it would make the simulation much, much faster. It would also reduce the amount of updates that need to be processed from thousands to just a few. Of course, applying a large amount of tile state changes at once is still a big undertaking, but in all actuality, the only thing we'll have to change at that point is the tile's active property, so that it renders differently. That means we reduce runtime so much that the simulation will probably be able to process a very large field of tiles in a very short time.

#### For the record, here's which tiles would have to be conditional:
- ButtonGridTile
- SemiConductorGridTile
- EmitterGridTile

And the rest is just connective and predictable. If it takes one input and is not interactive, then it is not conditional and thus can be precompiled.

Going for JIT preprocessing might be the most optimal way to go about making the program fast. It'll, in essence, make the hashtable lookup time negligible. Thus, the focus lies on the implementation of this feature now.