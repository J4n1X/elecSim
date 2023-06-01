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
  return {align(pos.x, 1.0f), align(pos.y, 1.0f)};
}

olc::vf2d Grid::CenterOfSquare(const olc::vf2d& squarePos) {
  return {squarePos.x + renderScale / 2, squarePos.y + renderScale / 2};
}

olc::vi2d Grid::TranslateIndex(const olc::vf2d& index,
                               const TileUpdateSide& side) {
  olc::vi2d targetIndex;
  switch (side) {
    case TileUpdateSide::Top:
      targetIndex = index - olc::vi2d(0, 1);
      break;
    case TileUpdateSide::Bottom:
      targetIndex = index + olc::vi2d(0, 1);
      break;
    case TileUpdateSide::Left:
      targetIndex = index - olc::vi2d(1, 0);
      break;
    case TileUpdateSide::Right:
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
  if (updates.size() > 0) {
    // Remove any pending updates
    updates.clear();
    // Reset the state of all tiles to the set default
    for ([[maybe_unused]] auto& [pos, tile] : tiles) {
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
    // Only draw if it's inside the drawing area
    if (!isRectangleOutside(tileScreenPos, {tileScreenSize, tileScreenSize},
                            {0, 0}, renderWindow)) {
      tile->Draw(renderer, tileScreenPos, tileScreenSize);
    }
  }
  if (highlightPos != nullptr) {
    // Clear the highlight layer (BEWARE, IF THIS IS 0 THE ENTIRE SCREEN IS
    // CLEARED ON EVERY DRAW CALL)
    renderer->SetDrawTarget((uint8_t)(uiLayer & 0x0F));
    renderer->Clear(olc::BLANK);
    auto corrPos = WorldToScreen(*highlightPos);
    olc::vi2d size = olc::vi2d((int)renderScale, (int)renderScale);
    renderer->DrawRect(corrPos, size, highlightColor);
  }
}

void Grid::Simulate() {
  decltype(updates) newUpdates;

  for (auto it = emitters.begin(); it != emitters.end();) {
    auto& tile = *it;
    if (tile.expired()) {
      it = emitters.erase(it);
    } else {
      updates.emplace(tile.lock()->GetPos(), TileUpdateFlags());
      it++;
    }
  }

  for (auto update : updates) {
    auto targetPosition = update.first;
    auto& target = tiles[targetPosition];
    auto updateSides = update.second;

    if (target == nullptr) throw "Nullptr in update target";
    auto outputSides = target->Simulate(updateSides);
    newUpdates.emplace(targetPosition, TileUpdateFlags());

    for (auto side : outputSides.GetFlags()) {
      auto flipped = TileUpdateFlags::FlipSide(side);
      try {
        olc::vi2d targetIndex = TranslateIndex(targetPosition, side);

        // TODO: Rewrite, accounting for the fact that using operator[] creates
        // the object if it does not exist
        if (newUpdates.find(targetIndex) !=
            newUpdates.end()) {  // Already exists
          newUpdates[targetIndex].SetFlag(flipped, true);
        } else {  // Create new entry if a tile exists at that index
          if (tiles.find(targetIndex) != tiles.end()) {
            auto flags = TileUpdateFlags(flipped);
            newUpdates[targetIndex] = flags;
          }
        }
      } catch ([[maybe_unused]] const char* err) {
        // std::cout << "Not setting update due to error: " << err <<
        // std::endl;
      }
    }
  }
  updates = newUpdates;
}