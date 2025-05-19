#include "Tiles.h"

// Just a helper function to get the points of the triangle
std::array<olc::vf2d, 3> GetTrianglePoints(olc::vf2d screenPos, int screenSize,
                                           TileFacingSide facing) {
  olc::vf2d p1, p2, p3;
  int halfWidth = screenSize / 4;  // Half of the half-width (1/4 of screenSize)
  int halfHeight =
      screenSize / 4;  // Half of the half-height (1/4 of screenSize)

  switch (facing) {
    case TileFacingSide::Top:
      p1 = olc::vf2d(screenPos.x + screenSize / 2, screenPos.y + halfHeight);
      p2 = olc::vf2d(screenPos.x + screenSize / 2 + halfWidth,
                     screenPos.y + screenSize / 2);
      p3 = olc::vf2d(screenPos.x + screenSize / 2 - halfWidth,
                     screenPos.y + screenSize / 2);
      break;
    case TileFacingSide::Right:
      p1 = olc::vf2d(screenPos.x + screenSize - halfWidth,
                     screenPos.y + screenSize / 2);
      p2 = olc::vf2d(screenPos.x + screenSize / 2,
                     screenPos.y + screenSize / 2 + halfHeight);
      p3 = olc::vf2d(screenPos.x + screenSize / 2,
                     screenPos.y + screenSize / 2 - halfHeight);
      break;
    case TileFacingSide::Bottom:
      p1 = olc::vf2d(screenPos.x + screenSize / 2,
                     screenPos.y + screenSize - halfHeight);
      p2 = olc::vf2d(screenPos.x + screenSize / 2 - halfWidth,
                     screenPos.y + screenSize / 2);
      p3 = olc::vf2d(screenPos.x + screenSize / 2 + halfWidth,
                     screenPos.y + screenSize / 2);
      break;
    case TileFacingSide::Left:
      p1 = olc::vf2d(screenPos.x + halfWidth, screenPos.y + screenSize / 2);
      p2 = olc::vf2d(screenPos.x + screenSize / 2,
                     screenPos.y + screenSize / 2 - halfHeight);
      p3 = olc::vf2d(screenPos.x + screenSize / 2,
                     screenPos.y + screenSize / 2 + halfHeight);
      break;
    default:
      std::cout << "Facing specified not found" << std::endl;
      throw "Facing specified not found";
  }
  return {p1, p2, p3};
}

TileFacingSide TileUpdateFlags::RotateToFacing(const TileFacingSide& side,
                                               const TileFacingSide& facing) {
  // Now, we could either loop until we get the right side, or we could
  // just use a lookup table. The lookup table is probably faster, so let's use
  // that.
  const static std::map<TileFacingSide, uint8_t> sideIndexTable = {
      {TileFacingSide::Top, 0},
      {TileFacingSide::Right, 1},
      {TileFacingSide::Bottom, 2},
      {TileFacingSide::Left, 3},
  };
  uint8_t sideIndex = sideIndexTable.at(side);
  uint8_t facingIndex = sideIndexTable.at(facing);
  uint8_t rotatedIndex = (sideIndex + facingIndex) % TILEUPDATESIDE_COUNT;
  // Now, we can just shift 1 over to the left by the rotated index
  auto rotatedFlag = static_cast<TileFacingSide>(1 << rotatedIndex);
  return rotatedFlag;
}

TileFacingSide TileUpdateFlags::FlipSide(const TileFacingSide& side) {
  switch (side) {
    case TileFacingSide::Top:
      return TileFacingSide::Bottom;
    case TileFacingSide::Bottom:
      return TileFacingSide::Top;
    case TileFacingSide::Left:
      return TileFacingSide::Right;
    case TileFacingSide::Right:
      return TileFacingSide::Left;
    default:
      throw "Side specified not found";
  }
}

std::string_view TileUpdateFlags::SideName(const TileFacingSide& side) {
  switch (side) {
    case TileFacingSide::Top:
      return "Top";
    case TileFacingSide::Bottom:
      return "Bottom";
    case TileFacingSide::Left:
      return "Left";
    case TileFacingSide::Right:
      return "Right";
    default:
      throw "Side specified not found";
  }
}

std::vector<TileFacingSide> TileUpdateFlags::GetFlags() {
  std::vector<TileFacingSide> ret;
  for (int i = 0; i < TILEUPDATESIDE_COUNT; i++) {
    if (flagValue & 1 << i) {
      ret.push_back(static_cast<TileFacingSide>(flagValue & 1 << i));
    }
  }
  return ret;
}

void GridTile::Draw(olc::PixelGameEngine* renderer, olc::vi2d screenPos,
                    int screenSize) {
  renderer->FillRect(screenPos, olc::vi2d(screenSize, screenSize),
                     activated ? activeColor : inactiveColor);
  // Get Triangle points into p1, p2, p3
  auto [p1, p2, p3] = GetTrianglePoints(screenPos, screenSize, facing);

  renderer->FillTriangle(p1, p2, p3, activated ? inactiveColor : activeColor);
}

// Calculating the size of the data here...
// 4 bytes - Tile ID
// 1 byte - Facing
// 8 bytes - Position
// 4 bytes - Size
// That's 13 bytes
// Surely there's a more clean way to do this, but this is easy.
std::array<char, GRIDTILE_BYTESIZE> GridTile::Serialize() {
  auto data = std::array<char, GRIDTILE_BYTESIZE>();
  *reinterpret_cast<int*>(data.data()) = GetTileId();
  *reinterpret_cast<TileFacingSide*>(data.data() + sizeof(int)) = facing;
  *reinterpret_cast<int*>(data.data() + sizeof(int) + sizeof(facing)) = pos.x;
  *reinterpret_cast<int*>(data.data() + sizeof(int) + sizeof(facing) +
                          sizeof(pos.x)) = pos.y;

  return data;
}

std::shared_ptr<GridTile> GridTile::Deserialize(
    std::array<char, GRIDTILE_BYTESIZE> data) {
  auto id = *reinterpret_cast<int*>(data.data());
  auto facing = *reinterpret_cast<TileFacingSide*>(data.data() + sizeof(id));
  auto posX =
      *reinterpret_cast<int*>(data.data() + sizeof(id) + sizeof(facing));
  auto posY = *reinterpret_cast<int*>(data.data() + sizeof(id) +
                                      sizeof(facing) + sizeof(posX));
  olc::vi2d pos = {posX, posY};
  std::shared_ptr<GridTile> tile;
  switch (id) {
    case 0:
      return std::make_shared<WireGridTile>(pos, facing, 1.0f);
    case 1:
      return std::make_shared<JunctionGridTile>(pos, facing, 1.0f);
    case 2:
      return std::make_shared<EmitterGridTile>(pos, facing, 1.0f);
    case 3:
      return std::make_shared<SemiConductorGridTile>(pos, facing, 1.0f);
    case 4:
      return std::make_shared<ButtonGridTile>(pos, facing, 1.0f);
    default:
      throw "Unknown tile ID";
  }
}

TileUpdateFlags WireGridTile::Simulate(TileUpdateFlags activatorSides) {
  // If the activation comes from the side we output to, we don't emit.
  // Same if there's no activator side
  if (!activatorSides.IsEmpty() && !activatorSides.GetFlag(this->facing)) {
    activated = true;
    auto sides = activatorSides.GetFlags();
    return TileUpdateFlags(this->facing);
  } else {
    activated = false;
    return TileUpdateFlags();
  }
}

void JunctionGridTile::Draw(olc::PixelGameEngine* renderer, olc::vi2d screenPos,
                            int screenSize) {
  renderer->FillRect(screenPos, olc::vi2d(screenSize, screenSize),
                     activated ? activeColor : inactiveColor);

  // Draw a smaller square in the center
  int halfSize = screenSize / 4;
  olc::vi2d centerPos = screenPos + olc::vi2d(screenSize / 2 - halfSize / 2,
                                              screenSize / 2 - halfSize / 2);
  renderer->FillRect(centerPos, olc::vi2d(halfSize, halfSize), olc::BLACK);
}

TileUpdateFlags JunctionGridTile::Simulate(TileUpdateFlags activatorSides) {
  if (!activatorSides.IsEmpty()) {
    auto sides = activatorSides.GetFlags();
    // Start by assuming we need to send to all sides
    auto outFlags = TileUpdateFlags::All();
    // And then remove the sides we came from
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
  if (!enabled) {
    return TileUpdateFlags();
  }
  auto outFlags = activated ? TileUpdateFlags(this->facing) : TileUpdateFlags();
  activated = !activated;
  return outFlags;
}

TileUpdateFlags EmitterGridTile::Interact() {
  enabled = !enabled;
  activated = enabled;
  return TileUpdateFlags();
}

TileUpdateFlags SemiConductorGridTile::Simulate(
    TileUpdateFlags activatorSides) {
  if (!activatorSides.IsEmpty()) {
    // Grab sides and rotate them to the facing
    auto sides = activatorSides.GetFlags();
    for (auto& side : sides) {
      auto rotatedSide = TileUpdateFlags::RotateToFacing(side, facing);
      // Now, if the rotated side is either left or right, we activate
      // ourselves
      if (rotatedSide == TileFacingSide::Left ||
          rotatedSide == TileFacingSide::Right) {
        activated = true;
      }
      // If the rotated side is the back, we emit
      if (rotatedSide == TileFacingSide::Bottom && activated) {
        activated = false;
        return TileUpdateFlags(this->facing);
      }
    }
  }
  // Should technically be unreachable, but just in case
  return TileUpdateFlags();
}

// This will prime the tile.
TileUpdateFlags SemiConductorGridTile::Interact() {
  activated = !activated;
  return TileUpdateFlags(
      TileUpdateFlags::RotateToFacing(TileFacingSide::Right, facing));
}

TileUpdateFlags ButtonGridTile::Simulate(
    [[maybe_unused]] TileUpdateFlags activatorSides) {
  if (activatorSides.IsEmpty()) {
    return TileUpdateFlags();
  }
  auto retVal = activated ? TileUpdateFlags(this->facing) : TileUpdateFlags();
  activated = false;
  return retVal;
}

TileUpdateFlags ButtonGridTile::Interact() {
  activated = true;
  return TileUpdateFlags::All();
}