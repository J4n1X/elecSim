#include "Grid.h"

TileUpdateSide FlipSide(TileUpdateSide side) {
  switch (side) {
    case TileUpdateSide::Top:
      return TileUpdateSide::Bottom;
    case TileUpdateSide::Bottom:
      return TileUpdateSide::Top;
    case TileUpdateSide::Left:
      return TileUpdateSide::Right;
    case TileUpdateSide::Right:
      return TileUpdateSide::Left;
    default:
      throw "Side specified not found";
  }
}

constexpr std::string_view SideName(TileUpdateSide side) {
  switch (side) {
    case TileUpdateSide::Top:
      return "Top";
    case TileUpdateSide::Bottom:
      return "Bottom";
    case TileUpdateSide::Left:
      return "Left";
    case TileUpdateSide::Right:
      return "Right";
    default:
      throw "Side specified not found";
  }
}

std::vector<TileUpdateSide> TileUpdateFlags::GetFlags() {
  std::vector<TileUpdateSide> ret;
  for (int i = 0; i < TILEUPDATESIDE_COUNT; i++) {
    if (flagValue & 1 << i) {
      ret.push_back(static_cast<TileUpdateSide>(flagValue & 1 << i));
    }
  }
  return ret;
}

Grid::Grid(olc::vi2d size, uint32_t renderScale, olc::vi2d renderOffset,
           int uiLayer, int gameLayer) {
  renderWindow = size;
  this->uiLayer = uiLayer;
  this->gameLayer = gameLayer;
  this->renderScale = renderScale;
  this->renderOffset = renderOffset;

  // add the brushes to the palette
  // palette->AddBrush<WireGridTile>("Wire");
  // palette->AddBrush<EmitterGridTile>("Pulse Emitter");
  // palette->SetBrush(0);
}

olc::vi2d Grid::TranslateIndex(olc::vi2d index, TileUpdateSide side) {
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
void Grid::Draw(olc::PixelGameEngine* renderer, olc::vi2d* highlightPos) {
  renderer->SetDrawTarget(gameLayer);
  renderer->Clear(backgroundColor);

  for (auto& tilePair : tiles) {
    auto& tile = tilePair.second;
    olc::vi2d tileScreenPos = WorldToScreen(tile->GetPos());
    int tileScreenSize = tile->GetSize() * renderScale;
    // Only draw if it's inside the drawing area
    if (!isRectangleOutside(tileScreenPos, {tileScreenSize, tileScreenSize},
                            {0, 0}, renderWindow)) {
      tile->Draw(renderer, tileScreenPos, tileScreenSize);
    }
  }
  if (highlightPos != nullptr) {
    // Clear the highlight layer (BEWARE, IF THIS IS 0 THE ENTIRE SCREEN IS
    // CLEARED ON EVERY DRAW CALL)
    renderer->SetDrawTarget(uiLayer);
    renderer->Clear(olc::BLANK);
    auto corrPos = WorldToScreen(*highlightPos);
    olc::vi2d size = olc::vi2d((int)renderScale, (int)renderScale);
    renderer->DrawRect(corrPos, size, highlightColor);
  }
}

void Grid::Simulate() {
  decltype(updates) newUpdates;

  for (auto it = alwaysUpdate.begin(); it != alwaysUpdate.end();) {
    auto& tile = *it;
    if (tile.expired()) {
      std::cout << "An alwaysUpdate-Tile has expired and will be dropped."
                << std::endl;
      it = alwaysUpdate.erase(it);
    } else {
      updates.emplace(tile.lock()->GetPos(), TileUpdateFlags());
      it++;
    }
  }

  for (auto update : updates) {
    auto pos = update.first;
    auto& target = tiles[pos];
    auto updateSides = update.second;

    if (target == nullptr) throw "Nullptr in update target";
    auto outputSides = target->Simulate(updateSides);
    newUpdates.emplace(pos, TileUpdateFlags());

    for (auto side : outputSides.GetFlags()) {
      // std::cout << "At " << pos << ": Triggering update on " <<
      // SideName(side)
      //           << std::endl;
      auto flipped = FlipSide(side);
      try {
        olc::vi2d targetIndex = TranslateIndex(pos, side);

        if (newUpdates.find(targetIndex) !=
            newUpdates.end()) {  // Already exists
          newUpdates[targetIndex].SetFlag(flipped, true);
        } else {  // Create new entry if a tile exists at that index
          if (tiles.find(targetIndex) != tiles.end()) {
            auto flags = TileUpdateFlags(flipped);
            newUpdates[targetIndex] = flags;
          }
        }
      } catch (const char* err) {
        // std::cout << "Not setting update due to error: " << err <<
        // std::endl;
      }
    }
  }
  updates = newUpdates;
}

void GridTile::Draw(olc::PixelGameEngine* renderer, olc::vi2d screenPos,
                    int screenSize) {
  renderer->FillRect(screenPos, olc::vi2d(screenSize, screenSize),
                     activated ? activeColor : inactiveColor);
}

TileUpdateFlags WireGridTile::Simulate(TileUpdateFlags activatorSides) {
  if (!activatorSides.IsEmpty()) {
    auto sides = activatorSides.GetFlags();
    auto outFlags = TileUpdateFlags::All();
    for (auto& side : sides) {
      outFlags.FlipFlag(side);
    }
    activated = true;
    return outFlags;
  } else {
    activated = false;
    return TileUpdateFlags();
  }
}

TileUpdateFlags EmitterGridTile::Simulate(
    [[maybe_unused]] TileUpdateFlags activatorSides) {
  auto outFlags = activated ? TileUpdateFlags::All() : TileUpdateFlags();
  activated = !activated;
  return outFlags;
}