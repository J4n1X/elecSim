#include "Grid.h"

#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>

namespace ElecSim {

// Implementation of SignalEdge::operator==
bool SignalEdge::operator==(const SignalEdge& other) const {
  return sourcePos == other.sourcePos && targetPos == other.targetPos;
}

// Implementation of SignalEdgeHash::operator()
std::size_t SignalEdgeHash::operator()(const SignalEdge& edge) const {
  using ankerl::unordered_dense::detail::wyhash::hash;
  return hash(&edge, sizeof(SignalEdge));
}

void Grid::QueueUpdate(std::shared_ptr<GridTile> tile,
                       const SignalEvent& event) {
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

int Grid::Simulate() {
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
#ifdef DEBUG
        std::cerr
            << "Warning: Maximum update limit reached (" << MAX_UPDATES
            << " updates). Enabling edge check to prevent potential cycle."
            << std::endl;
#else
        std::cout << "Update limit reached, enabling edge check to terminate "
                     "potential cycles."
                  << std::endl;
#endif
      }
      enableEdgeCheck = true;
    }

    auto update = updateQueue.front();
    updateQueue.pop();
    if (!update.tile) continue;
    if (enableEdgeCheck) {
      if (currentTickVisitedEdges.contains(
              SignalEdge{update.tile->GetPos(), update.event.sourcePos})) {
        // This edge has already been processed this tick, skip it
        continue;
      }
    }

#ifdef SIM_PREPROCESSING
    auto simObjMaybe = tileManager.GetSimulationObject(update.tile->GetPos());
    if (simObjMaybe.has_value()) {
      auto simObj = simObjMaybe.value();
      auto newSignals = simObj->ProcessSignal(update.event);
      for (const auto& newSignal : newSignals) {
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
#ifdef DEBUG
      std::cout
          << std::format(
                 "Warning: Processing update for unprocessed tile: {}->{}",
                 update.tile->GetPos(),
                 update.event.isActive ? "Active" : "Inactive")
          << std::endl;
#endif
      ProcessUpdateEvent(update);
    }

#else
    ProcessUpdateEvent(update);
#endif
    if (enableEdgeCheck) {
      currentTickVisitedEdges.insert(
          SignalEdge{update.tile->GetPos(), update.event.sourcePos});
    }

    updatesProcessed++;
  }
  return updatesProcessed;
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

// int Grid::Draw(olc::PixelGameEngine* renderer) {
//   if (!renderer) throw std::runtime_error("Grid has no renderer available");

//   // Clear Background
//   renderer->SetDrawTarget(gameLayer);
//   renderer->Clear(backgroundColor);

//   // Tiles exclusively render as decals.

//   // Draw tiles
//   // This spriteSize causes overdraw all the time, but without it, we get
//   // pixel gaps Update: Now that we use decals, this is a non-issue.

//   auto spriteSize = std::ceil(renderScale);
//   int drawnTiles = 0;

//   for (const auto& [pos, tile] : tiles) {
//     if (!tile) throw std::runtime_error("Grid contained entry with empty
//     tile");

//     olc::vf2d screenPos = WorldToScreenFloating(pos);
//     // Is this tile even visible?
//     if (screenPos.x + spriteSize <= 0 || screenPos.x >= renderWindow.x ||
//         screenPos.y + spriteSize <= 0 || screenPos.y >= renderWindow.y) {
//       continue;  // If not, why even draw it?
//     }

//     //tile->Draw(renderer, screenPos, spriteSize);
//     drawnTiles++;
//   }

//   // Highlight drawing has been moved to the Game class
//   return drawnTiles;
// }

void Grid::SetTile(vi2d pos, std::unique_ptr<GridTile> tile) {
  tile->SetPos(pos);
  auto [mapElement, inserted] = tiles.insert_or_assign(pos, std::move(tile));
  if (mapElement->second->IsEmitter()) {
    emitters.push_back(mapElement->second);
  }
  fieldIsDirty = true;  // Mark the field as modified
}
// TODO: These need to be moved into the game class or somewhere similar, like
// the RenderManager.
// olc::vf2d Grid::WorldToScreenFloating(const olc::vf2d& pos) {
//  return olc::vf2d((pos.x * renderScale) + renderOffset.x,
//                   (pos.y * renderScale) + renderOffset.y);
//}
//
// olc::vi2d Grid::WorldToScreen(const olc::vf2d& pos) {
//  auto screenPosFloating = WorldToScreenFloating(pos);
//  return olc::vi2d(static_cast<int>(std::floor(screenPosFloating.x)),
//                   static_cast<int>(std::floor(screenPosFloating.y)));
//}
//
// olc::vf2d Grid::ScreenToWorld(const olc::vi2d& pos) {
//  return olc::vf2d((pos.x - renderOffset.x) / renderScale,
//                   (pos.y - renderOffset.y) / renderScale);
//}
//

vi2d Grid::AlignToGrid(const vf2d& pos) {
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
#ifdef DEBUG
    std::cerr << "Error opening file for writing: " << filename << std::endl;
#endif
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
    file.write(chunkData.data(), chunkData.size());
    dataSize += chunkData.size();
  }
#ifdef DEBUG
  std::cout << std::format("Saved {} bytes to {}, total tiles: {}", dataSize,
                           filename, tiles.size())
            << std::endl;
#else
  (void)dataSize;
#endif
  file.close();
}

void Grid::Load(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
#ifdef DEBUG
    std::cerr << "Error opening file for reading: " << filename << std::endl;
#endif
    return;
  }

  Clear();
  size_t dataSize = 0;
  while (file) {
    std::array<char, GRIDTILE_BYTESIZE> data;
    file.read(data.data(), data.size());
    dataSize += file.gcount();
    if (file.gcount() == 0) break;

    std::unique_ptr<GridTile> tile = GridTile::Deserialize(data);
    auto [mapPair, _] = tiles.insert_or_assign(tile->GetPos(), std::move(tile));
    if (mapPair->second->IsEmitter()) {
      emitters.push_back(mapPair->second);
    }
  }

  file.close();
#ifdef DEBUG
  std::cout << std::format("Loaded {} bytes from {}, total {} tiles", dataSize,
                           filename, tiles.size())
            << std::endl;
#else
  (void)dataSize;
#endif
  fieldIsDirty = true;  // Mark the field as modified
  ResetSimulation();    // So that this preprocesses the tiles
}

}  // namespace ElecSim