#include "Grid.h"

#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <type_traits>

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

Grid::Grid(olc::vi2d size, float renderScale, olc::vi2d renderOffset,
           int uiLayer, int gameLayer)
    : renderWindow(size),
      uiLayer(uiLayer),
      gameLayer(gameLayer),
      renderScale(renderScale),
      renderOffset(renderOffset) {
  std::cout << std::format(
                   "Grid initialized with size: {}x{}, renderScale: {}, "
                   "renderOffset: {}",
                   size.x, size.y, renderScale, renderOffset)
            << std::endl;
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

  // We're now using signal chain tracking with visited positions to detect
  // circular updates. Each signal keeps track of all positions it has
  // visited, and we throw a runtime_error when we detect a cycle. However,
  // we'll still keep a reasonable upper limit as a safety measure:
  constexpr int MAX_UPDATES = 100000;
  while (!updateQueue.empty()) {
    // Safety check - prevent extremely long update chains
    if (updatesProcessed > MAX_UPDATES) {
      std::cerr << "Warning: Maximum update limit reached (" << MAX_UPDATES
                << " updates). This may indicate a complex circuit or "
                   "potential issue."
                << std::endl;
      break;
    }

    auto update = updateQueue.front();
    updateQueue.pop();
    if (!update.tile) continue;

#ifdef SIM_CACHING
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
            QueueUpdate(
                targetTile,
                SignalEvent(newSignal.sourcePos, FlipDirection(newSignal.fromDirection),
                            newSignal.isActive));
          }
        }
      }
    } else {
      // It's just a single object (probably a logic tile, but not necessarily)
      std::cout
          << std::format(
                 "Warning: Processing update for unprocessed tile: {}->{}",
                 update.tile->GetPos(),
                 update.event.isActive ? "Active" : "Inactive")
          << std::endl;
      ProcessUpdateEvent(update);
    }

#else
    ProcessUpdateEvent(update);
#endif
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
#ifdef SIM_CACHING
  if(fieldIsDirty) {
    std::cout << "Preprocessing tiles for simulation caching..." << std::endl;
    tileManager.Clear();
    tileManager.PreprocessTiles(tiles);
    fieldIsDirty = false;
  }
#endif
}

int Grid::Draw(olc::PixelGameEngine* renderer) {
  if (!renderer) throw std::runtime_error("Grid has no renderer available");

  // Clear Background
  renderer->SetDrawTarget(gameLayer);
  renderer->Clear(backgroundColor);

  // Tiles exclusively render as decals.

  // Draw tiles
  // This spriteSize causes overdraw all the time, but without it, we get
  // pixel gaps Update: Now that we use decals, this is a non-issue.

  auto spriteSize = std::ceil(renderScale);
  int drawnTiles = 0;
  for (const auto& [pos, tile] : tiles) {
    if (!tile) throw std::runtime_error("Grid contained entry with empty tile");

    olc::vf2d screenPos = WorldToScreenFloating(pos);
    // Is this tile even visible?
    if (screenPos.x + spriteSize <= 0 || screenPos.x >= renderWindow.x ||
        screenPos.y + spriteSize <= 0 || screenPos.y >= renderWindow.y) {
      continue;  // If not, why even draw it?
    }

    tile->Draw(renderer, screenPos, spriteSize);
    drawnTiles++;
  }

  // Highlight drawing has been moved to the Game class
  return drawnTiles;
}

void Grid::SetTile(olc::vf2d pos, std::unique_ptr<GridTile> tile,
                   bool emitter) {
  tiles.insert_or_assign(pos, std::move(tile));
  if (emitter) {
    emitters.push_back(tiles.at(pos));
  }
  auto initSignals = tiles.at(pos)->Init();
  for (const auto& signal : initSignals) {
    QueueUpdate(tiles.at(pos), signal);
  }

  fieldIsDirty = true;  // Mark the field as modified
}

void Grid::SetSelection(olc::vi2d startPos, olc::vi2d endPos) {
  // TODO: Implement this function instead of repeatedly calling SetTile
  (void)startPos;  // Avoid unused variable warning
  (void)endPos;    // Avoid unused variable warning
  throw std::runtime_error("SetSelection is not implemented yet");
}

olc::vf2d Grid::WorldToScreenFloating(const olc::vf2d& pos) {
  return olc::vf2d((pos.x * renderScale) + renderOffset.x,
                   (pos.y * renderScale) + renderOffset.y);
}

olc::vi2d Grid::WorldToScreen(const olc::vf2d& pos) {
  auto screenPosFloating = WorldToScreenFloating(pos);
  return olc::vi2d(static_cast<int>(std::floor(screenPosFloating.x)),
                   static_cast<int>(std::floor(screenPosFloating.y)));
}

olc::vf2d Grid::ScreenToWorld(const olc::vi2d& pos) {
  return olc::vf2d((pos.x - renderOffset.x) / renderScale,
                   (pos.y - renderOffset.y) / renderScale);
}

olc::vi2d Grid::AlignToGrid(const olc::vf2d& pos) {
  return olc::vf2d(static_cast<int>(std::floor(pos.x)),
                   static_cast<int>(std::floor(pos.y)));
}

olc::vf2d Grid::CenterOfSquare(const olc::vf2d& pos) {
  return olc::vf2d(std::floor(pos.x) + 0.5f, std::floor(pos.y) + 0.5f);
}

std::optional<std::shared_ptr<GridTile> const> Grid::GetTile(olc::vi2d pos) {
  auto tileIt = tiles.find(pos);
  return tileIt != tiles.end() ? std::optional{tileIt->second} : std::nullopt;
}

std::vector<std::weak_ptr<GridTile>> Grid::GetSelection(olc::vi2d startPos,
                                                        olc::vi2d endPos) {
  // ensure StartPos is actually the top-left corner
  olc::vi2d topLeft = startPos.min(endPos);
  olc::vi2d bottomRight = startPos.max(endPos);
  // lambda to select tiles in the area
  auto selectTilesInArea = [&](const auto& pair) {
    const auto& [pos, tile] = pair;
    return (pos.x >= topLeft.x && pos.x <= bottomRight.x &&
            pos.y >= topLeft.y && pos.y <= bottomRight.y);
  };
  auto filtered = tiles | std::ranges::views::filter(selectTilesInArea);
  std::vector<std::weak_ptr<GridTile>> selection = {};
  for (const auto& [_, tile] : filtered) {
    (void)_;
    // cast to weak
    selection.push_back(std::weak_ptr<GridTile>(tile));
  }
  return selection;
}

void Grid::Save(const std::string& filename) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Error opening file for writing: " << filename << std::endl;
    return;
  }

  size_t dataSize = 0;
  for (const auto& [pos, tile] : tiles) {
    auto data = tile->Serialize();
    dataSize += data.size();
    file.write(data.data(), data.size());
  }

  std::cout << std::format("Saved {} bytes to {}, total tiles: {}", dataSize,
                           filename, tiles.size())
            << std::endl;
  file.close();
}

void Grid::Load(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Error opening file for reading: " << filename << std::endl;
    return;
  }

  Clear();
  size_t dataSize = 0;
  while (file) {
    std::array<char, GRIDTILE_BYTESIZE> data;
    file.read(data.data(), data.size());
    dataSize += file.gcount();
    if (file.gcount() == 0) break;

    std::shared_ptr<GridTile> tile = std::move(GridTile::Deserialize(data));
    tiles.insert_or_assign(tile->GetPos(), tile);
    if (tile->IsEmitter()) {
      emitters.push_back(tile);
    }
  }

  file.close();

  std::cout << std::format("Loaded {} bytes from {}, total {} tiles", dataSize,
                           filename, tiles.size())
            << std::endl;
  fieldIsDirty = true;  // Mark the field as modified
  ResetSimulation(); // So that this preprocesses the tiles
}

}  // namespace ElecSim