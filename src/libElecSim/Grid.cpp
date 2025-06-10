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

// Implementation of PositionHash::operator()
std::size_t PositionHash::operator()(const olc::vi2d& pos) const {
  using ankerl::unordered_dense::detail::wyhash::hash;
  return hash(&pos, sizeof(olc::vi2d));
}

// Implementation of PositionEqual::operator()
bool PositionEqual::operator()(const olc::vi2d& lhs,
                               const olc::vi2d& rhs) const {
  return lhs == rhs;
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
                   "renderOffset: ({},{})",
                   size.x, size.y, renderScale, renderOffset.x, renderOffset.y)
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
          "Cycle detected in signal processing: edge from ({},{}) to "
          "({},{}). "
          "Offending signal side: {}; Total processed edges this tick: {}",
          edge.sourcePos.x, edge.sourcePos.y, edge.targetPos.x,
          edge.targetPos.y, DirectionToString(signal.fromDirection),
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
      // std::cout << std::format(
      //     "Processing update for tile at ({},{}): {}",
      //     update.tile->GetPos().x, update.tile->GetPos().y,
      //     update.event.isActive ? "Active" : "Inactive") << std::endl;
      //  If we have a simulation object, use it to process the update
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
                SignalEvent(targetPos, FlipDirection(newSignal.fromDirection),
                            newSignal.isActive));
          }
        }
      }
    } else {
      // It's just a single object (probably a logic tile, but not necessarily)
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
  tileManager.Clear();
  tileManager.PreprocessTiles(tiles);
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
}

void Grid::SetSelection(olc::vi2d startPos, olc::vi2d endPos) {
  // TODO: Implement this function instead of repeatedly calling SetTile
  (void)startPos;  // Avoid unused variable warning
  (void)endPos;    // Avoid unused variable warning
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
  ResetSimulation();
}

#ifdef SIM_CACHING
// Processes the signal of a group. This yields a vector of new signals by
// only simulating the input tile and simply cycling the state of the tiles
// inbetween the start and end.
// TODO: Try out how hard the performance is impacted if we run ProcessSignal
//       on all tiles inbetween, not just the input tile. Would yield accurate
//       probing results (gettile console command), but might also slow down
//       the simulation significantly.
std::vector<SignalEvent> TileGroupManager::SimulationGroup::ProcessSignal(
    const SignalEvent& signal) {
  auto newSignals = inputTile->ProcessSignal(signal);
  // TODO: Better checks.
  if (newSignals.empty()) {
    return {};  // No new signals produced
  }

  // Cycle the activation state of all inbetween tiles
  for (const auto& tile : inbetweenTiles) {
    // if (shouldFlip) {
    //   std::cout << "Flipping tile at "
    //             << tile->GetPos().x << "," << tile->GetPos().y << std::endl;
    //   tile->SetActivation(!tile->GetActivation());
    // }
    tile->SetActivation(!tile->GetActivation());
  }

  // Now, apply updates to the output tiles
  std::vector<SignalEvent> outputSignals;
  for (const auto& output : outputTiles) {
    Direction outputDir = DirectionFromVectors(
        output.inputterTile->GetPos(), output.tile->GetPos());
    auto signalEvent = SignalEvent(output.inputterTile->GetPos(), outputDir,
                                   output.inputterTile->GetActivation());
    auto tileSignals = output.tile->ProcessSignal(signalEvent);
    for (const auto& tileSignal : tileSignals) {
      outputSignals.push_back(tileSignal);
    }
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
  }
  // cut last newline
  if (!info.empty() && info.back() == '\n') {
    info.pop_back();
  }
  return info;
}

// Horribly ugly preprocessing function that builds paths like this:
// 1. Start with a tile that is deterministic and has no inputs, and has at
//    least one output.
// 2. Follow the deterministic path until we reach a tile that is not 
//    deterministic or until we reach a tile that has no outputs.
// 3. If the resulting group only contains one tile, we skip. 
// TODO: This is not just ugly, but it also does not work completely perfectly.
//       it does most the things correctly, but sometimes aborts 1-2 tiles too 
//       early. Honestly, I think this function has to be rewritten already.
//       We have the proof it works this way, now it needs to be done proper.
void TileGroupManager::PreprocessTiles(const TileMap& tiles) {
  // First pass: identify potential start tiles (tiles with outputs but no deterministic inputs)
  auto isValidStartTile = [&](const auto& pair) {
    const auto& [pos, tile] = pair;
    
    // Must have at least one output connection
    bool hasOutput =
        std::ranges::any_of(AllDirections, [&](const Direction& dir) {
          auto neighborPos = TranslatePosition(pos, dir);
          auto neighborIt = tiles.find(neighborPos);
          if (neighborIt == tiles.end()) return false;
          return neighborIt->second->CanReceiveFrom(FlipDirection(dir)) &&
                 tile->CanOutputTo(dir);
        });
    
    if (!hasOutput) return false;
    
    // Check if this tile has no deterministic inputs (valid start point)
    bool hasNoValidInput =
        std::ranges::none_of(AllDirections, [&](const Direction& dir) {
          auto neighborPos = TranslatePosition(pos, dir);
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
    
    // Don't reprocess already-handled tiles
    bool alreadyExists = simulationObjects.contains(tile->GetPos());
    return hasNoValidInput && !alreadyExists;
  };

  std::vector<std::shared_ptr<GridTile>> initialStartTiles;
  for (const auto& pair : tiles) {
    if (isValidStartTile(pair)) {
      initialStartTiles.push_back(pair.second);
    }
  }

  std::cout << "Initial start tiles to be processed: " << initialStartTiles.size()
            << std::endl;

  ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>> globalVisited;
  std::queue<std::shared_ptr<GridTile>> pendingStartTiles;
  
  // Add initial start tiles to queue
  for (auto& tile : initialStartTiles) {
    pendingStartTiles.push(tile);
  }

  // Process each potential start tile
  while (!pendingStartTiles.empty()) {
    auto inputTile = pendingStartTiles.front();
    pendingStartTiles.pop();
    
    // Skip if already processed
    if (globalVisited.contains(inputTile) || simulationObjects.contains(inputTile->GetPos())) {
      continue;
    }
    
    // Trace the deterministic path from this start tile
    std::vector<std::shared_ptr<GridTile>> pathTiles;
    std::vector<SimulationGroup::OutputTile> outputTiles;
    ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>> pathVisited;
    
    std::queue<std::shared_ptr<GridTile>> pathQueue;
    pathQueue.push(inputTile);
    
    while (!pathQueue.empty()) {
      auto current = pathQueue.front();
      pathQueue.pop();
      
      if (pathVisited.contains(current)) continue;
      pathVisited.insert(current);
      
      // If this is not a deterministic tile, handle it as a path endpoint
      if (!current->IsDeterministic()) {
        if (current != inputTile) {
          // This is an endpoint of our deterministic path
          // Find which tile fed into it
          std::shared_ptr<GridTile> inputterTile = nullptr;
          for (const auto& dir : AllDirections) {
            if (!current->CanReceiveFrom(dir)) continue;
            
            auto sourcePos = TranslatePosition(current->GetPos(), dir);
            auto sourceIt = tiles.find(sourcePos);
            if (sourceIt == tiles.end()) continue;
            
            auto& sourceTile = sourceIt->second;
            if (sourceTile->CanOutputTo(FlipDirection(dir)) && 
                pathVisited.contains(sourceTile)) {
              inputterTile = sourceTile;
              break;
            }
          }
          
          if (inputterTile) {
            outputTiles.emplace_back(current, inputterTile);
          }
        }
        
        // Queue up neighbors as potential new start tiles
        for (const auto& dir : AllDirections) {
          if (!current->CanOutputTo(dir)) continue;
          
          auto neighborPos = TranslatePosition(current->GetPos(), dir);
          auto neighborIt = tiles.find(neighborPos);
          if (neighborIt != tiles.end() &&
              neighborIt->second->CanReceiveFrom(FlipDirection(dir)) &&
              !globalVisited.contains(neighborIt->second)) {
            pendingStartTiles.push(neighborIt->second);
          }
        }
        continue;
      }
      
      // This is a deterministic tile - add to path
      if (current != inputTile) {
        pathTiles.push_back(current);
      }
      
      // Find all valid neighbors
      for (const auto& dir : AllDirections) {
        if (!current->CanOutputTo(dir)) continue;
        
        auto neighborPos = TranslatePosition(current->GetPos(), dir);
        auto neighborIt = tiles.find(neighborPos);
        if (neighborIt == tiles.end()) continue;
        
        auto& neighbor = neighborIt->second;
        if (!neighbor->CanReceiveFrom(FlipDirection(dir))) continue;
        
        // Count how many tiles can output to this neighbor
        int inputCount = std::ranges::count_if(AllDirections, [&](const Direction& inDir) {
          auto sourcePos = TranslatePosition(neighbor->GetPos(), inDir);
          auto sourceIt = tiles.find(sourcePos);
          if (sourceIt == tiles.end()) return false;
          return sourceIt->second->CanOutputTo(FlipDirection(inDir)) &&
                 neighbor->CanReceiveFrom(inDir);
        });
        
        if (inputCount > 1) {
          // Multiple inputs - treat as output tile and potential new start point
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
    
    // Mark all tiles in this path as visited
    globalVisited.insert(inputTile);
    for (auto& tile : pathTiles) {
      globalVisited.insert(tile);
    }
    
    // Create simulation object
    if (pathTiles.empty() && outputTiles.empty()) {
      // Single tile with no deterministic path - Drop!
#ifdef DEBUG
        std::cout << std::format(
            "Tile at ({},{}) has no deterministic path, skipping.",
            inputTile->GetPos().x, inputTile->GetPos().y) << std::endl;
#endif
    } else {
      // Create simulation group
      auto simGroup = std::make_unique<SimulationGroup>(
          inputTile, std::move(pathTiles), std::move(outputTiles));
      
      auto [_, inserted] = simulationObjects.emplace(
          inputTile->GetPos(), std::move(simGroup));
      
      if (!inserted) {
#ifdef DEBUG
        std::cerr << "Warning: Tile Group starting at ("
                  << inputTile->GetPos().x << ", " << inputTile->GetPos().y
                  << ") already exists in simulationObjects, skipping."
                  << std::endl;
#endif
      }
    }
  }
  
  // Second pass: ensure all remaining tiles are covered
  for (const auto& [pos, tile] : tiles) {
    if (!globalVisited.contains(tile) && !simulationObjects.contains(pos)) {
      // This tile wasn't processed in any group - create a single tile simulation object
      simulationObjects.emplace(pos, std::make_unique<SimulationTile>(tile));
      globalVisited.insert(tile);
    }
  }

#ifdef DEBUG
  for (const auto& [pos, obj] : simulationObjects) {
    std::cout << obj->GetObjectInfo() << '\n';
  }
#endif
  std::cout << "Preprocessing complete, total simulation objects: "
            << simulationObjects.size() << std::endl;
}

#endif  // SIM_CACHING

}  // namespace ElecSim