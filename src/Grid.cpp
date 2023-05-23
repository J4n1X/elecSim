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

const char* SideName(TileUpdateSide side) {
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

Grid::Grid(olc::vi2d size, int uiLayer, int gameLayer) {
  dimensions = size;
  this->uiLayer = uiLayer;
  this->gameLayer = gameLayer;

  // add the brushes to the palette
  //palette->AddBrush<WireGridTile>("Wire");
  //palette->AddBrush<EmitterGridTile>("Pulse Emitter");
  //palette->SetBrush(0);
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
  if (targetIndex.x < dimensions.x && targetIndex.y < dimensions.y &&
      targetIndex.x >= 0 && targetIndex.y >= 0) {  // if not out of bounds
    return targetIndex;
  } else {
    throw "Index out of bounds";
  }
}

void GridTile::Draw(olc::PixelGameEngine* renderer) {
  auto corrPos = Grid::WorldToScreen(pos);
  auto corrSize = size * Grid::worldScale;
  renderer->FillRect(corrPos, olc::vi2d(corrSize, corrSize),
                     activated ? activeColor : inactiveColor);
}

// void GridTile::Highlight(olc::PixelGameEngine* renderer) {
//   auto rectSize = olc::vi2d(size - 1, size - 1);
//   renderer->DrawRect(pos, rectSize, highlightColor);
// }

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

int Grid::worldScale = 0;
olc::vi2d Grid::worldOffset = olc::vi2d(0, 0);

void Grid::Draw(olc::PixelGameEngine* renderer, olc::vi2d* highlightPos) {
  renderer->SetDrawTarget(gameLayer);
  renderer->Clear(backgroundColor);
  for (auto& tile : tiles) {
    tile.second->Draw(renderer);
  }
  if (highlightPos != nullptr) {
    // Clear the highlight layer (BEWARE, IF THIS IS 0 THE ENTIRE SCREEN IS
    // CLEARED ON EVERY DRAW CALL)
    renderer->SetDrawTarget(uiLayer);
    renderer->Clear(olc::BLANK);
    auto corrPos = WorldToScreen(*highlightPos);
    olc::vi2d size = olc::vi2d((int)worldScale, (int)worldScale);
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
    auto target = tiles[pos];
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
        // std::cout << "Not setting update due to error: " << err << std::endl;
      }
    }
  }
  updates = newUpdates;
}