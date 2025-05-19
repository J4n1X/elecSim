#include "Tiles.h"

#include <array>
#include <map>
#include <stdexcept>

// Helper function to get the points of the triangle for a tile
std::array<olc::vf2d, 3> GetTrianglePoints(olc::vf2d screenPos, int screenSize,
                                           TileFacingSide facing) {
  constexpr int kHalfDiv = 4;
  int halfWidth = screenSize / kHalfDiv;
  int halfHeight = screenSize / kHalfDiv;
  olc::vf2d center = {screenPos.x + screenSize / 2,
                      screenPos.y + screenSize / 2};

  // Offsets for each facing: {p1_offset, p2_offset, p3_offset}
  static const std::map<TileFacingSide, std::array<olc::vf2d, 3>> offsets = {
      {TileFacingSide::Top,
       {olc::vf2d(0, -halfHeight), olc::vf2d(halfWidth, 0),
        olc::vf2d(-halfWidth, 0)}},
      {TileFacingSide::Right,
       {olc::vf2d(halfWidth, 0), olc::vf2d(0, halfHeight),
        olc::vf2d(0, -halfHeight)}},
      {TileFacingSide::Bottom,
       {olc::vf2d(0, halfHeight), olc::vf2d(-halfWidth, 0),
        olc::vf2d(halfWidth, 0)}},
      {TileFacingSide::Left,
       {olc::vf2d(-halfWidth, 0), olc::vf2d(0, -halfHeight),
        olc::vf2d(0, halfHeight)}}};

  auto it = offsets.find(facing);
  if (it == offsets.end()) {
    throw std::runtime_error("Facing specified not found");
  }
  const auto& offs = it->second;
  return {center + offs[0], center + offs[1], center + offs[2]};
}

TileFacingSide TileUpdateFlags::RotateToFacing(const TileFacingSide& side,
                                               const TileFacingSide& facing) {
  static const std::map<TileFacingSide, uint8_t> sideIndexTable = {
      {TileFacingSide::Top, 0},
      {TileFacingSide::Right, 1},
      {TileFacingSide::Bottom, 2},
      {TileFacingSide::Left, 3},
  };
  uint8_t sideIndex = sideIndexTable.at(side);
  uint8_t facingIndex = sideIndexTable.at(facing);
  uint8_t rotatedIndex = (sideIndex + facingIndex) % TILEUPDATESIDE_COUNT;
  return static_cast<TileFacingSide>(1 << rotatedIndex);
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
      throw std::runtime_error("Side specified not found");
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
      throw std::runtime_error("Side specified not found");
  }
}

std::vector<TileFacingSide> TileUpdateFlags::GetFlags() {
  std::vector<TileFacingSide> ret;
  for (int i = 0; i < TILEUPDATESIDE_COUNT; ++i) {
    if (flagValue & (1 << i)) {
      ret.push_back(static_cast<TileFacingSide>(1 << i));
    }
  }
  return ret;
}

void GridTile::Draw(olc::PixelGameEngine* renderer, olc::vi2d screenPos,
                    int screenSize) {
  renderer->FillRect(screenPos, olc::vi2d(screenSize, screenSize),
                     activated ? activeColor : inactiveColor);
  auto [p1, p2, p3] = GetTrianglePoints(screenPos, screenSize, facing);
  renderer->FillTriangle(p1, p2, p3, activated ? inactiveColor : activeColor);
}

// Serialization: 4 bytes - Tile ID, 1 byte - Facing, 8 bytes - Position, 4
// bytes - Size
std::array<char, GRIDTILE_BYTESIZE> GridTile::Serialize() {
  std::array<char, GRIDTILE_BYTESIZE> data{};
  *reinterpret_cast<int*>(data.data()) = GetTileId();
  *reinterpret_cast<TileFacingSide*>(data.data() + sizeof(int)) = facing;
  *reinterpret_cast<int*>(data.data() + sizeof(int) + sizeof(facing)) = pos.x;
  *reinterpret_cast<int*>(data.data() + sizeof(int) + sizeof(facing) +
                          sizeof(pos.x)) = pos.y;
  return data;
}

std::shared_ptr<GridTile> GridTile::Deserialize(
    std::array<char, GRIDTILE_BYTESIZE> data) {
  int id = *reinterpret_cast<int*>(data.data());
  TileFacingSide facing =
      *reinterpret_cast<TileFacingSide*>(data.data() + sizeof(id));
  int posX = *reinterpret_cast<int*>(data.data() + sizeof(id) + sizeof(facing));
  int posY = *reinterpret_cast<int*>(data.data() + sizeof(id) + sizeof(facing) +
                                     sizeof(posX));
  olc::vi2d pos = {posX, posY};
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
      throw std::runtime_error("Unknown tile ID");
  }
}

TileUpdateFlags WireGridTile::Simulate(TileUpdateFlags activatorSides) {
  if (!activatorSides.IsEmpty() && !activatorSides.GetFlag(this->facing)) {
    activated = !activated;
    return TileUpdateFlags(this->facing);
  }
  activated = false;
  return TileUpdateFlags();
}

void JunctionGridTile::Draw(olc::PixelGameEngine* renderer, olc::vi2d screenPos,
                            int screenSize) {
  renderer->FillRect(screenPos, olc::vi2d(screenSize, screenSize),
                     activated ? activeColor : inactiveColor);
  int halfSize = screenSize / 4;
  olc::vi2d centerPos = screenPos + olc::vi2d(screenSize / 2 - halfSize / 2,
                                              screenSize / 2 - halfSize / 2);
  renderer->FillRect(centerPos, olc::vi2d(halfSize, halfSize), olc::BLACK);
}

TileUpdateFlags JunctionGridTile::Simulate(TileUpdateFlags activatorSides) {
  if (!activatorSides.IsEmpty()) {
    auto outFlags = TileUpdateFlags::All();
    auto sideFlags = activatorSides.GetFlags();
    for(auto &sideFlag : sideFlags) {
      outFlags.SetFlag(sideFlag, false);
    }
    activated = !activated;
    return outFlags;
  }
  return TileUpdateFlags();
}

TileUpdateFlags EmitterGridTile::Simulate(
    [[maybe_unused]] TileUpdateFlags activatorSides) {
  if (enabled) {
    activated = !activated;
    return TileUpdateFlags(this->facing);
  } else {
    // Signals are controlled by on/off impulses, so we need to check which
    // pulse we're on
    auto returnFlags =
        activated ? TileUpdateFlags(this->facing) : TileUpdateFlags();
    activated = false;
    return returnFlags;
  }
}

TileUpdateFlags EmitterGridTile::Interact() {
  enabled = !enabled;
  return TileUpdateFlags();
}

TileUpdateFlags SemiConductorGridTile::Simulate(
    TileUpdateFlags activatorSides) {
  // Save previous states.
  bool prevActivated = activated;
  bool prevEnabled = enabled;

  if (!activatorSides.IsEmpty()) {
    for (const auto& side : activatorSides.GetFlags()) {
      auto rotatedSide = TileUpdateFlags::RotateToFacing(side, facing);
      if (rotatedSide == TileFacingSide::Left ||
          rotatedSide == TileFacingSide::Right) {
        enabled = !enabled;
        break;
      }
      if (rotatedSide == TileFacingSide::Bottom) {
        activated = !activated;
        break;
      }
    }
    if (enabled && activated) {
      return TileUpdateFlags(this->facing);
    }
    if((prevActivated && prevEnabled) && (prevActivated != activated || prevEnabled != enabled)) {
      return TileUpdateFlags(this->facing);
    }
  }
  return TileUpdateFlags();
}

TileUpdateFlags SemiConductorGridTile::Interact() {
  enabled = !activated;
  return TileUpdateFlags();
}

TileUpdateFlags ButtonGridTile::Simulate(
    [[maybe_unused]] TileUpdateFlags activatorSides) {
  if (activatorSides.IsEmpty()) return TileUpdateFlags();
  return TileUpdateFlags(this->facing);
}

TileUpdateFlags ButtonGridTile::Interact() {
  activated = !activated;
  return TileUpdateFlags::All();
}