#include "TileGroupManager.h"

#include <format>
#include <iostream>
#include <ranges>

#include "Common.h"

namespace ElecSim {

#ifdef SIM_PREPROCESSING

// Processes the signal of a group. This yields a vector of new signals by
// only simulating the input tile and simply cycling the state of the tiles
// inbetween the start and end.
// TODO: Try out how hard the performance is impacted if we run ProcessSignal
//       on all tiles inbetween, not just the input tile. Would yield accurate
//       probing results (gettile console command), but might also slow down
//       the simulation significantly.
// UPDATE: This would be really fucking slow. There'll be a better way, later.
std::vector<SignalEvent> TileGroupManager::SimulationGroup::ProcessSignal(
    const SignalEvent& signal) {
  auto newSignals = inputTile->ProcessSignal(signal);
  if (newSignals.empty()) {
    return {};  // No new signals produced
  }

  // Cycle the activation state of all inbetween tiles
  for (const auto& tile : inbetweenTiles) {
    tile->SetActivation(!tile->GetActivation());
  }

  // Now, apply updates to the output tiles
  std::vector<SignalEvent> outputSignals;
  for (const auto& output : outputTiles) {
    Direction outputDir = DirectionFromVectors(output.inputterTile->GetPos(),
                                               output.tile->GetPos());
    auto signalEvent = SignalEvent(output.inputterTile->GetPos(), outputDir,
                                   output.inputterTile->GetActivation());
    outputSignals.push_back(signalEvent);
  }
  return outputSignals;
}

std::string TileGroupManager::SimulationGroup::GetObjectInfo() const {
  std::string info = "SimulationTileGroup:\n";
  info += "  Input Tile:\n  " + inputTile->GetTileInformation() + "\n";
  info += "  Inbetween Tiles:\n";
  for (const auto& tile : inbetweenTiles) {
    info += "    " + tile->GetTileInformation() + '\n';
  }
  info += "  Output Tiles:\n";
  for (const auto& output : outputTiles) {
    info += "    " + output.tile->GetTileInformation() + '\n';
  }  // cut last newline
  if (!info.empty() && info.back() == '\n') {
    info.pop_back();
  }
  return info;
}

// Helper functions for tile preprocessing
bool TileGroupManager::HasOutputConnection(
    const std::shared_ptr<GridTile>& tile, const TileMap& tiles) const {
  return std::ranges::any_of(AllDirections, [&](const Direction& dir) {
    auto neighborPos = TranslatePosition(tile->GetPos(), dir);
    auto neighborIt = tiles.find(neighborPos);
    if (neighborIt == tiles.end()) return false;
    return neighborIt->second->CanReceiveFrom(FlipDirection(dir)) &&
           tile->CanOutputTo(dir);
  });
}

bool TileGroupManager::HasDeterministicInputs(
    const std::shared_ptr<GridTile>& tile, const TileMap& tiles) const {
  return std::ranges::any_of(AllDirections, [&](const Direction& dir) {
    auto neighborPos = TranslatePosition(tile->GetPos(), dir);
    auto neighborIt = tiles.find(neighborPos);
    if (neighborIt == tiles.end()) return false;

    // Only consider connections where neighbor can output to this tile
    if (!neighborIt->second->CanOutputTo(FlipDirection(dir)) ||
        !tile->CanReceiveFrom(dir)) {
      return false;
    }

    // For start tile detection, only deterministic tiles matter
    return neighborIt->second->IsDeterministic();
  });
}

bool TileGroupManager::IsValidStartTile(const std::shared_ptr<GridTile>& tile,
                                        const TileMap& tiles) const {
  // Must have at least one output connection
  if (!HasOutputConnection(tile, tiles)) {
    return false;
  }

  // Check if this tile has no deterministic inputs (valid start point)
  bool hasNoValidInput = !HasDeterministicInputs(tile, tiles);

  // Don't reprocess already-handled tiles
  bool alreadyExists = simulationObjects.contains(tile->GetPos());
  return hasNoValidInput && !alreadyExists;
}

std::vector<std::shared_ptr<GridTile>> TileGroupManager::FindInitialStartTiles(
    const TileMap& tiles) const {
  std::vector<std::shared_ptr<GridTile>> startTiles;
  for (const auto& [pos, tile] : tiles) {
    if (IsValidStartTile(tile, tiles)) {
      startTiles.push_back(tile);
    }
  }
  return startTiles;
}

int TileGroupManager::CountInputsToTile(
    const std::shared_ptr<GridTile>& neighbor, const TileMap& tiles) const {
  return std::ranges::count_if(AllDirections, [&](const Direction& inDir) {
    auto sourcePos = TranslatePosition(neighbor->GetPos(), inDir);
    auto sourceIt = tiles.find(sourcePos);
    if (sourceIt == tiles.end()) return false;
    return sourceIt->second->CanOutputTo(FlipDirection(inDir)) &&
           neighbor->CanReceiveFrom(inDir);
  });
}

std::shared_ptr<GridTile> TileGroupManager::FindInputterTile(
    const std::shared_ptr<GridTile>& tile, const TileMap& tiles,
    const ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>&
        pathVisited) const {
  for (const auto& dir : AllDirections) {
    if (!tile->CanReceiveFrom(dir)) continue;

    auto sourcePos = TranslatePosition(tile->GetPos(), dir);
    auto sourceIt = tiles.find(sourcePos);
    if (sourceIt == tiles.end()) continue;

    auto& sourceTile = sourceIt->second;
    if (sourceTile->CanOutputTo(FlipDirection(dir)) &&
        pathVisited.contains(sourceTile)) {
      return sourceTile;
    }
  }
  return nullptr;
}

void TileGroupManager::QueueNeighborsAsStartTiles(
    const std::shared_ptr<GridTile>& tile, const TileMap& tiles,
    std::queue<std::shared_ptr<GridTile>>& pendingStartTiles,
    const ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>&
        globalVisited) const {
  for (const auto& dir : AllDirections) {
    if (!tile->CanOutputTo(dir)) continue;

    auto neighborPos = TranslatePosition(tile->GetPos(), dir);
    auto neighborIt = tiles.find(neighborPos);
    if (neighborIt != tiles.end() &&
        neighborIt->second->CanReceiveFrom(FlipDirection(dir)) &&
        !globalVisited.contains(neighborIt->second)) {
      pendingStartTiles.push(neighborIt->second);
    }
  }
}

void TileGroupManager::ProcessDeterministicTileNeighbors(
    const std::shared_ptr<GridTile>& current, const TileMap& tiles,
    std::queue<std::shared_ptr<GridTile>>& pathQueue,
    std::vector<SimulationGroup::OutputTile>& outputTiles,
    std::queue<std::shared_ptr<GridTile>>& pendingStartTiles,
    const ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>&
        globalVisited) const {
  for (const auto& dir : AllDirections) {
    if (!current->CanOutputTo(dir)) continue;

    auto neighborPos = TranslatePosition(current->GetPos(), dir);
    auto neighborIt = tiles.find(neighborPos);
    if (neighborIt == tiles.end()) continue;

    auto& neighbor = neighborIt->second;
    if (!neighbor->CanReceiveFrom(FlipDirection(dir))) continue;

    int inputCount = CountInputsToTile(neighbor, tiles);

    if (inputCount > 1) {
// Multiple inputs - treat the tile pushing into it as an output tile and
// push the new tile as a start tile
#ifdef DEBUG
      std::cout << std::format(
                       "TileGroupManager::ProcessDeterministicTileNeighbors: "
                       "Tile at {} has multiple inputs, treating as output "
                       "tile with inputter {}.",
                       neighbor->GetPos(), current->GetPos())
                << std::endl;
#endif
      outputTiles.emplace_back(neighbor, current);
      if (!globalVisited.contains(neighbor)) {
        pendingStartTiles.push(neighbor);
      }
    } else if (neighbor->IsDeterministic()) {
      // Single input deterministic tile - continue path
      pathQueue.push(neighbor);
    } else {
      // Single input non-deterministic tile - end path here
      outputTiles.emplace_back(neighbor, current);
      if (!globalVisited.contains(neighbor)) {
        pendingStartTiles.push(neighbor);
      }
    }
  }
}

TileGroupManager::PathTraceResult TileGroupManager::TraceDeterministicPath(
    const std::shared_ptr<GridTile>& inputTile, const TileMap& tiles,
    std::queue<std::shared_ptr<GridTile>>& pendingStartTiles,
    const ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>&
        globalVisited) const {
  PathTraceResult result;
  std::queue<std::shared_ptr<GridTile>> pathQueue;
  pathQueue.push(inputTile);

  while (!pathQueue.empty()) {
    auto current = pathQueue.front();
    pathQueue.pop();

    if (result.pathVisited.contains(current)) continue;
    result.pathVisited.insert(current);

    // If this is not a deterministic tile, handle it as a path endpoint
    if (!current->IsDeterministic()) {
      if (current != inputTile) {
        // This is an endpoint of our deterministic path
        auto inputterTile =
            FindInputterTile(current, tiles, result.pathVisited);
        if (inputterTile) {
          result.outputTiles.emplace_back(current, inputterTile);
        }
      }

      // Queue up neighbors as potential new start tiles
      QueueNeighborsAsStartTiles(current, tiles, pendingStartTiles,
                                 globalVisited);
      continue;
    }

    // This is a deterministic tile - add to path
    if (current != inputTile) {
      result.pathTiles.push_back(current);
    }

    // Process neighbors
    ProcessDeterministicTileNeighbors(current, tiles, pathQueue,
                                      result.outputTiles, pendingStartTiles,
                                      globalVisited);
  }

  return result;
}

void TileGroupManager::CreateSimulationObject(
    const std::shared_ptr<GridTile>& inputTile,
    std::vector<std::shared_ptr<GridTile>> pathTiles,
    std::vector<SimulationGroup::OutputTile> outputTiles) {
  if (pathTiles.empty() && outputTiles.empty()) {
    // Single tile with no deterministic path - create a SimulationTile
    simulationObjects.emplace(inputTile->GetPos(),
                              std::make_unique<SimulationTile>(inputTile));
#ifdef DEBUG
    std::cout << std::format(
                     "Tile at {} has no deterministic path, creating single "
                     "tile simulation.",
                     inputTile->GetPos())
              << std::endl;
#endif
  } else {
    // Create simulation group
    auto simGroup = std::make_unique<SimulationGroup>(
        inputTile, std::move(pathTiles), std::move(outputTiles));

    auto [_, inserted] =
        simulationObjects.emplace(inputTile->GetPos(), std::move(simGroup));

    if (!inserted) {
#ifdef DEBUG
      std::cerr << "Warning: Tile Group starting at (" << inputTile->GetPos().x
                << ", " << inputTile->GetPos().y
                << ") already exists in simulationObjects, skipping."
                << std::endl;
#endif
    }
  }
}

void TileGroupManager::CoverRemainingTiles(
    const TileMap& tiles,
    ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>&
        globalVisited) {
  for (const auto& [pos, tile] : tiles) {
    if (!globalVisited.contains(tile) && !simulationObjects.contains(pos)) {
      // This tile wasn't processed in any group - create a single tile
      // simulation object
      simulationObjects.emplace(pos, std::make_unique<SimulationTile>(tile));
      globalVisited.insert(tile);
    }
  }
}

// Main preprocessing function - now much cleaner and easier to follow
void TileGroupManager::PreprocessTiles(const TileMap& tiles) {
  // Find all potential start tiles
  auto initialStartTiles = FindInitialStartTiles(tiles);
  ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>
      globalVisited;
  std::queue<std::shared_ptr<GridTile>> pendingStartTiles;

  // Add initial start tiles to processing queue
  for (const auto& tile : initialStartTiles) {
    pendingStartTiles.push(tile);
  }

  // Process each potential start tile
  while (!pendingStartTiles.empty()) {
    auto inputTile = pendingStartTiles.front();
    pendingStartTiles.pop();

    // Skip if already processed
    if (globalVisited.contains(inputTile) ||
        simulationObjects.contains(inputTile->GetPos())) {
      continue;
    }

    // Trace the deterministic path from this start tile
    auto pathResult = TraceDeterministicPath(inputTile, tiles,
                                             pendingStartTiles, globalVisited);

    // Mark all tiles in this path as visited
    globalVisited.insert(inputTile);
    for (auto& tile : pathResult.pathTiles) {
      globalVisited.insert(tile);
    }

    // Create appropriate simulation object
    CreateSimulationObject(inputTile, std::move(pathResult.pathTiles),
                           std::move(pathResult.outputTiles));
  }

  // Ensure all remaining tiles are covered
  CoverRemainingTiles(tiles, globalVisited);

#ifdef DEBUG
  for (const auto& [pos, obj] : simulationObjects) {
    std::cout << obj->GetObjectInfo() << '\n';
  }
  std::cout << std::flush;
  std::cout << "Preprocessing complete, total simulation objects: "
            << simulationObjects.size() << std::endl;
#endif
}

#endif  // SIM_PREPROCESSING

}  // namespace ElecSim
