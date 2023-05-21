#include "Grid.h"

static TileUpdateSide FlipSide(TileUpdateSide side) {
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

static const char* SideName(TileUpdateSide side) {
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

void GridTile::Draw(olc::PixelGameEngine* renderer) {
  renderer->FillRect(pos, olc::vi2d(size, size),
                     activated ? activeColor : inactiveColor);
}

void GridTile::Highlight(olc::PixelGameEngine* renderer) {
  auto rectSize = olc::vi2d(size - 1, size - 1);
  renderer->DrawRect(pos, rectSize, highlightColor);
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
};

TileUpdateFlags EmitterGridTile::Simulate(TileUpdateFlags activatorSides) {
  auto outFlags = activated ? TileUpdateFlags::All() : TileUpdateFlags();
  activated = !activated;
  return outFlags;
};

Grid::Grid(olc::vi2d size, int tileSize) {
  dimensions = size;
  this->tileSize = tileSize;
  tiles.resize(dimensions.y);
  for (int y = 0; y < dimensions.y; y++) {
    tiles[y].resize(dimensions.x);
    for (int x = 0; x < dimensions.x; x++) {
      auto pos = olc::vi2d(x * tileSize, y * tileSize);
      tiles[y][x] = std::make_unique<EmptyGridTile>(pos, tileSize);
      drawQueue.push_back(olc::vi2d(x, y));
    }
  }
}

void Grid::Draw(olc::PixelGameEngine* renderer, olc::vi2d* highlightIndex,
                uint32_t gameLayer, uint32_t uiLayer) {
  // for (int y = 0; y < dimensions.y; y++) {
  //   for (int x = 0; x < dimensions.x; x++) {
  //     auto& tile = tiles[y][x];
  //     tile->Draw(renderer);
  //     if (highlightIndex != nullptr) {
  //       if (olc::vi2d(x, y) == *highlightIndex) {
  //         tile->Highlight(renderer);
  //       }
  //     }
  //   }
  // }

  for (auto& tileIndex : drawQueue) {
    renderer->SetDrawTarget(gameLayer);
    tiles[tileIndex.y][tileIndex.x]->Draw(renderer);
  }
  if (highlightIndex != nullptr) {
    // Clear the highlight layer (BEWARE, IF THIS IS 0 THE ENTIRE SCREEN IS
    // CLEARED ON EVERY DRAW CALL)
    renderer->SetDrawTarget(uiLayer);
    renderer->Clear(olc::BLANK);
    tiles[highlightIndex->y][highlightIndex->x]->Highlight(renderer);
    renderer->SetDrawTarget(nullptr);  // return to default drawtarget
  }
  drawQueue.clear();
}

void Grid::Simulate() {
  decltype(updates) newUpdates;

  for (auto& update : alwaysUpdate) {
    updates.emplace(update, TileUpdateFlags());
  }

  for (auto& update : updates) {
    auto pos = update.first;
    auto& target = tiles[pos.y][pos.x];
    auto updateSides = update.second;

    auto outputSides = target->Simulate(updateSides);
    newUpdates.emplace(pos, TileUpdateFlags());
    drawQueue.push_back(pos);

    for (auto& side : outputSides.GetFlags()) {
      // std::cout << "At " << pos << ": Triggering update on " <<
      // SideName(side)
      //           << std::endl;
      auto flipped = FlipSide(side);
      olc::vi2d targetIndex;
      switch (side) {
        case TileUpdateSide::Top:
          targetIndex = pos - olc::vi2d(0, 1);
          break;
        case TileUpdateSide::Bottom:
          targetIndex = pos + olc::vi2d(0, 1);
          break;
        case TileUpdateSide::Left:
          targetIndex = pos - olc::vi2d(1, 0);
          break;
        case TileUpdateSide::Right:
          targetIndex = pos + olc::vi2d(1, 0);
          break;
      }
      if (pos.x < dimensions.x &&
          pos.y < dimensions.y) {  // if not out of bounds
        if (newUpdates.find(targetIndex) !=
            newUpdates.end()) {  // Already exists
          newUpdates[targetIndex].SetFlag(flipped);
        } else {  // Create new entry
          auto flags = TileUpdateFlags(flipped);
          newUpdates.emplace(targetIndex, flags);
        }
      }
    }
  }
  updates = newUpdates;
}