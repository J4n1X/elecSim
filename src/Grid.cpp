#include "Grid.h"

#include <cmath>

Grid::Grid(olc::vi2d size, float renderScale, olc::vi2d renderOffset,
           int uiLayer, int gameLayer) {
  renderWindow = size;
  this->uiLayer = uiLayer;
  this->gameLayer = gameLayer;
  this->renderScale = renderScale;
  this->renderOffset = renderOffset;
}

olc::vi2d Grid::WorldToScreen(const olc::vf2d& pos) {
  int x = (int)((pos.x - renderOffset.x) * renderScale);
  int y = (int)((pos.y - renderOffset.y) * renderScale);
  return olc::vi2d(x, y);
}

olc::vf2d Grid::ScreenToWorld(const olc::vi2d& pos) {
  auto x = (float)pos.x / renderScale + renderOffset.x;
  auto y = (float)pos.y / renderScale + renderOffset.y;
  return olc::vf2d(x, y);
}

void Grid::ZoomToMouse(const olc::vf2d& mouseWorldPosBefore,
                       const olc::vf2d& mouseWorldPosAfter) {
  renderOffset += mouseWorldPosBefore - mouseWorldPosAfter;
}

olc::vf2d Grid::AlignToGrid(const olc::vf2d& pos) {
  auto align = [](std::floating_point auto num, std::floating_point auto base) {
    auto quotient = num / base;
    auto rounded = floor(quotient);
    return rounded * base;
  };
  return {align(pos.x, 1.0), align(pos.y, 1.0)};
}

olc::vf2d Grid::CenterOfSquare(const olc::vf2d& squarePos) {
  return {squarePos.x + renderScale / 2, squarePos.y + renderScale / 2};
}

olc::vi2d Grid::TranslateIndex(const olc::vf2d& index,
                               const TileFacingSide& side) {
  olc::vi2d targetIndex;
  switch (side) {
    case TileFacingSide::Top:
      targetIndex = index - olc::vi2d(0, 1);
      break;
    case TileFacingSide::Bottom:
      targetIndex = index + olc::vi2d(0, 1);
      break;
    case TileFacingSide::Left:
      targetIndex = index - olc::vi2d(1, 0);
      break;
    case TileFacingSide::Right:
      targetIndex = index + olc::vi2d(1, 0);
      break;
  }
  return targetIndex;
}

// Helper for Grid::Draw
static bool isRectangleOutside(const olc::vi2d& rectPos1,
                               const olc::vi2d& rectSize1,
                               const olc::vi2d& rectPos2,
                               const olc::vi2d& rectSize2) {
  // Calculate the coordinates of the corners of the rectangles
  int rect1Left = rectPos1.x;
  int rect1Right = rectPos1.x + rectSize1.x;
  int rect1Top = rectPos1.y;
  int rect1Bottom = rectPos1.y + rectSize1.y;

  int rect2Left = rectPos2.x;
  int rect2Right = rectPos2.x + rectSize2.x;
  int rect2Top = rectPos2.y;
  int rect2Bottom = rectPos2.y + rectSize2.y;

  return (rect1Right <= rect2Left) || (rect1Left >= rect2Right) ||
         (rect1Bottom <= rect2Top) || (rect1Top >= rect2Bottom);
}

void Grid::ResetSimulation() {
  if (!updates.empty()) {
    updates.clear();
    for (auto& [pos, tile] : tiles) {
      tile->ResetActivation();
    }
  }
}

void Grid::Draw(olc::PixelGameEngine* renderer, olc::vf2d* highlightPos) {
  renderer->SetDrawTarget((uint8_t)(gameLayer & 0x0F));
  renderer->Clear(backgroundColor);

  for (auto& tilePair : tiles) {
    auto& tile = tilePair.second;
    olc::vi2d tileScreenPos = WorldToScreen(tile->GetPos());
    auto tileScreenSize = (int)(tile->GetSize() * renderScale);
    if (!isRectangleOutside(tileScreenPos, {tileScreenSize, tileScreenSize},
                            {0, 0}, renderWindow)) {
      tile->Draw(renderer, tileScreenPos, tileScreenSize);
    }
  }
  if (highlightPos != nullptr) {
    renderer->SetDrawTarget((uint8_t)(uiLayer & 0x0F));
    renderer->Clear(olc::BLANK);
    auto corrPos = WorldToScreen(*highlightPos);
    olc::vi2d size = olc::vi2d((int)renderScale, (int)renderScale);
    renderer->DrawRect(corrPos, size, highlightColor);
  }
}

void Grid::QueueUpdate(olc::vi2d pos, TileUpdateFlags flags) {
  updates.emplace(pos, flags);
}

void Grid::Simulate() {
  decltype(updates) newUpdates;

  for (auto it = emitters.begin(); it != emitters.end();) {
    if (it->expired()) {
      it = emitters.erase(it);
    } else {
      QueueUpdate(it->lock()->GetPos(), TileUpdateFlags());
      ++it;
    }
  }

  for (auto update : updates) {
    auto targetPosition = update.first;
    auto& target = tiles[targetPosition];
    auto updateSides = update.second;

    if (target == nullptr) throw "Nullptr in update target";
    auto outputSides = target->Simulate(updateSides);
    if (outputSides.IsEmpty()) continue;
    newUpdates.emplace(targetPosition, TileUpdateFlags());

    for (auto side : outputSides.GetFlags()) {
      auto flipped = TileUpdateFlags::FlipSide(side);
      try {
        olc::vi2d targetIndex = TranslateIndex(targetPosition, side);
        if (newUpdates.find(targetIndex) != newUpdates.end()) {
          newUpdates.at(targetIndex).SetFlag(flipped, true);
        } else {
          if (tiles.find(targetIndex) != tiles.end()) {
            auto flags = TileUpdateFlags(flipped);
            newUpdates.emplace(targetIndex, flags);
          }
        }
      } catch ([[maybe_unused]] const char* err) {
      }
    }
  }
  updates = newUpdates;
}

void Grid::Save(const std::string& filename) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Error opening file for writing: " << filename << std::endl;
    return;
  }
  for (auto& tilePair : tiles) {
    auto& tile = tilePair.second;
    auto serializedData = tile->Serialize();
    file.write(reinterpret_cast<const char*>(serializedData.data()),
               serializedData.size());
  }
  file.close();
}

void Grid::Load(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Error opening file for reading: " << filename << std::endl;
    return;
  }
  tiles.clear();
  emitters.clear();
  while (file) {
    std::array<char, GRIDTILE_BYTESIZE> data;
    file.read(data.data(), data.size());
    if (file.gcount() == 0) break;
    auto tile = GridTile::Deserialize(data);
    tiles.insert_or_assign(tile->GetPos(), tile);
    if (tile->IsEmitter()) {
      emitters.push_back(tile);
    }
  }
  file.close();
}