#include "Grid.h"

#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include "Common.h"

namespace ElecSim {

bool SignalEdge::operator==(const SignalEdge& other) const {
  return sourcePos == other.sourcePos && targetPos == other.targetPos;
}

std::size_t SignalEdgeHash::operator()(const SignalEdge& edge) const {
  using ankerl::unordered_dense::detail::wyhash::hash;
  return hash(&edge, sizeof(SignalEdge));
}

void Grid::QueueUpdate(std::shared_ptr<GridTile> tile,
                       const SignalEvent& event) noexcept {
  updateQueue.push(UpdateEvent(tile, event, currentTick));
}

void Grid::ProcessUpdateEvent(const UpdateEvent& updateEvent) {
  auto newSignals = updateEvent.tile->ProcessSignal(updateEvent.event);
  // Queue up new signals
  for (const auto& signal : newSignals) {
    auto targetPos = TranslatePosition(signal.sourcePos,
                                       FlipDirection(signal.fromDirection));

    // Create signal edge
    SignalEdge edge{signal.sourcePos, targetPos};

    // Check if this edge has been traversed in this tick
    if (false && currentTickVisitedEdges.contains(edge)) {
      // This would create a cycle - throw an exception
      // If we didn't, and just skipped, it would result in false behavior.
      throw std::runtime_error(std::format(
          "Cycle detected in signal processing: edge from {} to "
          "{}. "
          "Offending signal side: {}; Total processed edges this tick: {}",
          edge.sourcePos, edge.targetPos,
          DirectionToString(signal.fromDirection),
          currentTickVisitedEdges.size()));
    }

    // Record this edge as visited

    auto targetTileIt = tiles.find(targetPos);
    if (targetTileIt == tiles.end()) continue;

    auto& targetTile = targetTileIt->second;
    if (targetTile->CanReceiveFrom(signal.fromDirection)) {
      currentTickVisitedEdges.insert(edge);
      // Create a simpler signal event (no visited positions)
      QueueUpdate(targetTile,
                  SignalEvent(targetPos, FlipDirection(signal.fromDirection),
                              signal.isActive));
    }
  }
}

Grid::SimulationResult Grid::Simulate() {
  if (fieldIsDirty) {
    DebugPrint("Grid is dirty on attempted simulation step, resetting simulation.");
    ResetSimulation();
  }

  SimulationResult simResult;
  int updatesProcessed = 0;
  currentTick++;  // Increment the tick counter

  // Clear edge tracking for this simulation tick
  currentTickVisitedEdges.clear();

  // Queue updates from emitters first
  for (auto it = emitters.begin(); it != emitters.end();) {
    if (it->expired()) {
      it = emitters.erase(it);
      continue;
    }

    auto tile = std::dynamic_pointer_cast<EmitterGridTile>(it->lock());
    if (tile && tile->ShouldEmit(currentTick)) {
      tile->SetActivation(!tile->GetActivation());  // Toggle emitter state
      // Now using the simpler SignalEvent constructor
      QueueUpdate(tile, SignalEvent(tile->GetPos(), tile->GetFacing(),
                                    tile->GetActivation()));
      simResult.affectedTiles.insert(
          TileStateChange{tile->GetPos(), tile->GetActivation()});
    }
    ++it;
  }

  constexpr int MAX_UPDATES = 100000;

  // While false by default, if a large amount of updates are processed
  // this tick, we turn it on to detect potential cycles and terminate them.
  bool enableEdgeCheck = false;
  while (!updateQueue.empty()) {
    // Safety check - prevent extremely long update chains
    if (updatesProcessed > MAX_UPDATES) {
      if (!enableEdgeCheck) {
        DebugPrint("Warning: Maximum update limit reached ({} updates). Enabling edge check to prevent potential cycle.", 
                  MAX_UPDATES);
      }
      enableEdgeCheck = true;
    }

    auto update = updateQueue.front();
    updateQueue.pop();
    if (!update.tile) continue;
    if (enableEdgeCheck) {
      if (currentTickVisitedEdges.contains(
              SignalEdge{update.tile->GetPos(), update.event.sourcePos})) {
        throw std::runtime_error(
            std::format("Cycle detected in signal processing: edge from {} to "
                        "{}. Offending signal side: {}",
                        update.tile->GetPos(), update.event.sourcePos,
                        DirectionToString(update.event.fromDirection)));
      }
    }

#ifdef SIM_PREPROCESSING
    auto simObjMaybe = tileManager.GetSimulationObject(update.tile->GetPos());
    if (simObjMaybe.has_value()) {
      auto simObj = simObjMaybe.value();
      auto processResult = simObj->ProcessSignal(update.event);

      // Insert into affected tiles 
      simResult.affectedTiles.insert(processResult.affectedTiles.begin(),
                           processResult.affectedTiles.end());

      for (const auto& newSignal : processResult.newSignals) {
        // Queue the new signal events
        auto targetPos = TranslatePosition(
            newSignal.sourcePos, FlipDirection(newSignal.fromDirection));
        auto targetTileIt = tiles.find(targetPos);
        if (targetTileIt != tiles.end()) {
          auto& targetTile = targetTileIt->second;
          if (targetTile->CanReceiveFrom(newSignal.fromDirection)) {
            QueueUpdate(targetTile,
                        SignalEvent(newSignal.sourcePos,
                                    FlipDirection(newSignal.fromDirection),
                                    newSignal.isActive));
          }
        }
      }
    } else {
      // It's just a single object (probably a logic tile, but not necessarily)
      DebugPrint("Warning: Processing update for unprocessed tile: {}->{}",
               update.tile->GetPos(),
               update.event.isActive ? "Active" : "Inactive");
      ProcessUpdateEvent(update);
      simResult.affectedTiles.insert(
          TileStateChange{update.tile->GetPos(), update.tile->GetActivation()});
    }

#else
    ProcessUpdateEvent(update);
    affectedTiles.insert(
          TileStateChange{update.tile->GetPos(), update.tile->GetActivation()});
#endif
    if (enableEdgeCheck) {
      currentTickVisitedEdges.insert(
          SignalEdge{update.tile->GetPos(), update.event.sourcePos});
    }

    updatesProcessed++;
  }
  simResult.updatesProcessed = updatesProcessed;
  return simResult;
}

void Grid::ResetSimulation() {
  // Reset tick counter
  currentTick = 0;
  // Clear the update queue
  if (!updateQueue.empty()) {
    updateQueue = std::queue<UpdateEvent>();
  }
  currentTickVisitedEdges.clear();

  // Reset all tiles
  for (auto& [pos, tile] : tiles) {
    tile->ResetActivation();
    auto initState = tile->Init();
    if (initState.empty()) continue;  // No initial state to process
    for (const auto& event : initState) {
      QueueUpdate(tile, event);
    }
  }
#ifdef SIM_PREPROCESSING
  if (fieldIsDirty) {
    tileManager.Clear();
    tileManager.PreprocessTiles(tiles);
    fieldIsDirty = false;
  }
#endif
}

void ElecSim::Grid::SetTile(vi2d pos, std::shared_ptr<GridTile> tile) {
  tile->SetPos(pos);
  auto [mapElement, inserted] = tiles.insert_or_assign(pos, tile);
  if (mapElement->second->IsEmitter()) {
    emitters.push_back(mapElement->second);
  }
  fieldIsDirty = true;  // Mark the field as modified
}

void Grid::InteractWithTile(vi2d pos) noexcept {
  if (std::optional tileOpt = GetTile(pos)) {
    auto tile = tileOpt.value();
    auto newSignals = tile->Interact();
    for (const auto& signal : newSignals) {
      QueueUpdate(tile, signal);
    }
  }
}

vi2d Grid::AlignToGrid(const vf2d& pos) noexcept {
  return vi2d(static_cast<int>(std::floor(pos.x)),
              static_cast<int>(std::floor(pos.y)));
}

std::optional<std::shared_ptr<GridTile> const> Grid::GetTile(vi2d pos) {
  auto tileIt = tiles.find(pos);
  return tileIt != tiles.end() ? std::optional{tileIt->second} : std::nullopt;
}

std::vector<std::weak_ptr<GridTile>> Grid::GetSelection(vi2d startPos,
                                                        vi2d endPos) {
  vi2d topLeft = startPos.min(endPos);
  vi2d bottomRight = startPos.max(endPos);

  // Replace this complex view chain with a simple loop
  std::vector<std::weak_ptr<GridTile>> result;
  for (const auto& [pos, tile] : tiles) {
    if (pos.x >= topLeft.x && pos.x <= bottomRight.x && pos.y >= topLeft.y &&
        pos.y <= bottomRight.y) {
      result.emplace_back(tile);
    }
  }
  return result;
}

void Grid::Save(const std::string& filename) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    DebugPrint("Error opening file for writing: {}", filename);
    return;
  }

  size_t dataSize = 0;
  auto serializedTileData =
      tiles | std::views::values | std::views::transform([](const auto& tile) {
        return tile->Serialize();
      }) |
      std::views::transform([](const auto& data) -> std::vector<char> {
        return std::vector<char>(data.begin(), data.end());
      }) |
      std::views::chunk(1000);
  for (const auto& chunk : serializedTileData) {
    auto chunkData =
        chunk | std::views::join | std::ranges::to<std::vector<char>>();
    file.write(chunkData.data(),
               static_cast<std::streamsize>(chunkData.size()));
    dataSize += chunkData.size();
  }
  DebugPrint("Saved {} bytes to {}, total tiles: {}", dataSize, filename, tiles.size());
  (void)dataSize; // Silence unused variable warning in release mode
  file.close();
}

void Grid::Load(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    DebugPrint("Error opening file for reading: {}", filename);
    return;
  }

  Clear();
  size_t dataSize = 0;
  while (file) {
    std::array<char, GRIDTILE_BYTESIZE> data;
    file.read(data.data(), data.size());
    if (file.gcount() < 0) {
      throw std::runtime_error(
          std::format("Error reading from file: {}. Invalid read count: {}",
                      filename, file.gcount()));
    }
    dataSize += static_cast<size_t>(file.gcount());
    if (file.gcount() == 0) break;

    std::unique_ptr<GridTile> tile = GridTile::Deserialize(data);
    auto [mapPair, _] = tiles.insert_or_assign(tile->GetPos(), std::move(tile));
    if (mapPair->second->IsEmitter()) {
      emitters.push_back(mapPair->second);
    }
  }

  file.close();
  DebugPrint("Loaded {} bytes from {}, total {} tiles", dataSize, filename, tiles.size());
  (void)dataSize; // Silence unused variable warning in release mode
  fieldIsDirty = true;  // Mark the field as modified
  ResetSimulation();    // So that this preprocesses the tiles
}

}  // namespace ElecSim