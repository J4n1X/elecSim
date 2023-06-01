#include "Tiles.h"

TileUpdateSide TileUpdateFlags::FlipSide(const TileUpdateSide& side) {
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

std::string_view TileUpdateFlags::SideName(const TileUpdateSide& side) {
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