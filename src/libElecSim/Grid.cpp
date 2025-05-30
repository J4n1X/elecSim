#include "Grid.h"

#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>

namespace ElecSim {

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

void Grid::QueueUpdate(std::shared_ptr<GridTile> tile, const SignalEvent& event,
                       int priority) {
  updateQueue.push(UpdateEvent(tile, event, priority));
}

olc::vi2d Grid::TranslatePosition(olc::vi2d pos, Direction dir) const {
  switch (dir) {
    case Direction::Top:
      return pos + olc::vi2d(0, -1);
    case Direction::Right:
      return pos + olc::vi2d(1, 0);
    case Direction::Bottom:
      return pos + olc::vi2d(0, 1);
    case Direction::Left:
      return pos + olc::vi2d(-1, 0);
    default:
      return pos;
  }
}

void Grid::ProcessSignalEvent(const SignalEvent& event) {
  auto tileIt = tiles.find(event.sourcePos);
  if (tileIt == tiles.end()) return;

  auto& tile = tileIt->second;
  auto newSignals = tile->ProcessSignal(event);

  // Queue up new signals
  for (const auto& signal : newSignals) {
    auto targetPos = TranslatePosition(signal.sourcePos,
                                       FlipDirection(signal.fromDirection));
    auto targetTileIt = tiles.find(targetPos);
    if (targetTileIt == tiles.end()) continue;

    auto& targetTile = targetTileIt->second;
    if (targetTile->CanReceiveFrom(signal.fromDirection)) {
      if (targetTile->GetActivation() == signal.isActive) {
        // We are pushing an active signal into an already active tile.
        // This is needed for some logic, like when a wire is connected to two
        // sides which both give it an input signal. You don't want to turn it
        // off if one of the signals is turned off. However, this enables
        // infinite energy loops... Hold on, just got a great idea, I gotta test
        // it.
      }

      QueueUpdate(targetTile, SignalEvent(targetPos, signal.fromDirection,
                                          signal.isActive));
    }
  }
}

int Grid::Simulate() {
  int updatesProcessed = 0;
  currentTick++;  // Increment the tick counter

  // Queue updates from emitters first
  for (auto it = emitters.begin(); it != emitters.end();) {
    if (it->expired()) {
      it = emitters.erase(it);
      continue;
    }

    auto tile = std::dynamic_pointer_cast<EmitterGridTile>(it->lock());
    if (tile && tile->ShouldEmit(currentTick)) {
      tile->SetActivation(!tile->GetActivation());  // Toggle emitter state
      QueueUpdate(
          tile,
          SignalEvent(tile->GetPos(), tile->GetFacing(), tile->GetActivation()),
          1);
    }
    ++it;
  }

  // FIXME: There are situations in which circular updates can occur, freezing
  // the simulation. That's just how this simulation system works, because each
  // update only addresses the tiles next to it, instead of building a full tree
  // of updates. When I first implemented it that way, the idea was still that
  // this would be a pulse simulation. There is no easy fix for this at all.
  // An idea would be to give the update a origin tile, and if the update is circular
  // then we could ignore it.
  // For now, this dirty solution just breaks up the simulation after 100k updates.

  while (!updateQueue.empty()) {
    if(updatesProcessed > 100000){
      std::cerr << "Warning: Too many updates processed, breaking to avoid infinite loop." << std::endl;
      break;  // Prevent infinite loops
    }
    auto update = updateQueue.top();
    updateQueue.pop();
    if (!update.tile) continue;
    ProcessSignalEvent(update.event);
    updatesProcessed++;
  }
  return updatesProcessed;
}

void Grid::ResetSimulation() {
  // Clear the update queue
  while (!updateQueue.empty()) {
    updateQueue.pop();
  }

  // Reset tick counter
  currentTick = 0;

  // Reset all tiles
  for (auto& [pos, tile] : tiles) {
    tile->ResetActivation();

    // Queue "off" signals from all directions with highest priority (100)
    // This ensures all inputs to each tile are properly initialized
    for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
      Direction dir = static_cast<Direction>(i);
      if (tile->CanReceiveFrom(dir)) {
        QueueUpdate(tile, SignalEvent(pos, dir, false), 100);
      }
    }
  }
}

int Grid::Draw(olc::PixelGameEngine* renderer) {
  if (!renderer) throw std::runtime_error("Grid has no renderer available");

  // Clear Background
  renderer->SetDrawTarget(gameLayer);
  renderer->Clear(backgroundColor);

  // Tiles exclusively render as decals.

  // Draw tiles
  // This spriteSize causes overdraw all the time, but without it, we get pixel
  // gaps
  // Update: Now that we use decals, this is a non-issue.
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

}  // namespace ElecSim