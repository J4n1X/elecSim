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

// --- TileUpdateFlags ---
TileFacingSide TileUpdateFlags::RotateSideToFacing(
    const TileFacingSide& side, const TileFacingSide& facing) {
  static const std::map<TileFacingSide, uint8_t> sideIndexTable = {
      {TileFacingSide::Top, 0},
      {TileFacingSide::Right, 1},
      {TileFacingSide::Bottom, 2},
      {TileFacingSide::Left, 3},
  };
  auto sideIt = sideIndexTable.find(side);
  auto facingIt = sideIndexTable.find(facing);
  if (sideIt == sideIndexTable.end() || facingIt == sideIndexTable.end()) {
    throw std::runtime_error("Invalid side or facing in RotateSideToFacing");
  }
  uint8_t sideIndex = sideIt->second;
  uint8_t facingIndex = facingIt->second;
  uint8_t rotatedIndex = (sideIndex + facingIndex) % TILEUPDATESIDE_COUNT;
  return static_cast<TileFacingSide>(1 << rotatedIndex);
}

TileUpdateFlags TileUpdateFlags::RotateToFacing(TileUpdateFlags flags,
                                                const TileFacingSide& facing) {
  TileUpdateFlags ret;
  for (const auto& flag : flags.GetFlags()) {
    auto rotatedFlag = RotateSideToFacing(flag, facing);
    ret.SetFlag(rotatedFlag, true);
  }
  return ret;
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

// --- GridTile ---
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
  std::shared_ptr<GridTile> ret_ptr;
  switch (id) {
    case 0:
      ret_ptr = std::make_shared<WireGridTile>();
      break;
    case 1:
      ret_ptr = std::make_shared<JunctionGridTile>();
      break;
    case 2:
      ret_ptr = std::make_shared<EmitterGridTile>();
      break;
    case 3:
      ret_ptr = std::make_shared<SemiConductorGridTile>();
      break;
    case 4:
      ret_ptr = std::make_shared<ButtonGridTile>();
      break;
    default:
      throw std::runtime_error("Unknown tile ID");
  }
  ret_ptr->SetPos(pos);
  ret_ptr->SetFacing(facing);
  return ret_ptr;
}

TileUpdateFlags WireGridTile::Simulate(TileUpdateFlags activatorSides) {
  if (!activatorSides.IsEmpty()) {
    for (const auto& side : activatorSides.GetFlags()) {
      if (inputSides.GetFlag(side)) {
        activated = !activated;
        return TileUpdateFlags(this->facing);
      }
    }
  }
  activated = false;
  return TileUpdateFlags();
}

TileUpdateFlags JunctionGridTile::Simulate(TileUpdateFlags activatorSides) {
  if (!activatorSides.IsEmpty()) {
    auto outFlags = TileUpdateFlags::All();
    auto sideFlags = activatorSides.GetFlags();
    for (auto& sideFlag : sideFlags) {
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
  if (!activatorSides.IsEmpty()) {
    for (const auto& side : activatorSides.GetFlags()) {
      auto rotatedSide = TileUpdateFlags::RotateSideToFacing(side, facing);
      if (rotatedSide == TileFacingSide::Left ||
          rotatedSide == TileFacingSide::Right) {
        internalState ^= 0b01;
      }
      if (rotatedSide == TileFacingSide::Bottom) {
        internalState ^= 0b10;
      }
    }
    if (internalState == 3) {
      activated = true;
      return TileUpdateFlags(facing);
    } else if (internalState < 3 && activated) {
      activated = false;
      return TileUpdateFlags(facing);
    }
  }
  return TileUpdateFlags();
}

TileUpdateFlags SemiConductorGridTile::Interact() {
  internalState ^= 1;
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