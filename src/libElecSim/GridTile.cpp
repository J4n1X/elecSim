#include "GridTile.h"
#include "GridTileTypes.h"
#include <sstream>

namespace ElecSim {

const std::string_view TileTypeToString(TileType type) {
  switch (type) {
    case TileType::Wire:
      return "Wire";
    case TileType::Junction:
      return "Junction";
    case TileType::Emitter:
      return "Emitter";
    case TileType::SemiConductor:
      return "SemiConductor";
    case TileType::Button:
      return "Button";
    case TileType::Inverter:
      return "Inverter";
    case TileType::Crossing:
      return "Crossing";
    default:
      return "Unknown";
  }
}

// --- GridTile Implementation ---

GridTile::GridTile(vi2d newPos, Direction newFacing,
                   bool newDefaultActivation)
    : pos(newPos),
      facing(newFacing),
      activated(newDefaultActivation),
      defaultActivation(newDefaultActivation),
      canReceive{},
      canOutput{},
      inputStates{} {
  // Initialize I/O arrays
}

GridTile::GridTile(const GridTile& other) {
  pos = other.pos;
  facing = other.facing;
  activated = other.activated;
  defaultActivation = other.defaultActivation;
  canReceive = other.canReceive;
  canOutput = other.canOutput;
  inputStates = other.inputStates;
}

GridTile::GridTile(GridTile&& other) noexcept
    : pos(std::move(other.pos)),
      facing(other.facing),
      activated(other.activated),
      defaultActivation(other.defaultActivation),
      canReceive(std::move(other.canReceive)),
      canOutput(std::move(other.canOutput)),
      inputStates(std::move(other.inputStates)) {
  // reset other to a default state
  other.facing = Direction::Top;
  other.activated = false;
  other.defaultActivation = false;
  other.canReceive.fill(false);
  other.canOutput.fill(false);
  other.inputStates.fill(false);
  other.pos = {0, 0};
}

GridTile& GridTile::operator=(const GridTile& other) {
  if (this == &other) [[unlikely]]
    return *this;  // Self-assignment check
  pos = other.pos;
  facing = other.facing;
  activated = other.activated;
  defaultActivation = other.defaultActivation;
  canReceive = other.canReceive;
  canOutput = other.canOutput;
  inputStates = other.inputStates;
  return *this;
}



//void GridTile::Draw(olc::PixelGameEngine* renderer, olc::vf2d screenPos,
//                    float screenSize, int alpha) {
//  // Create local copies of colors with the specified alpha
//  olc::Pixel drawActiveColor = activeColor;
//  olc::Pixel drawInactiveColor = inactiveColor;
//  drawActiveColor.a = alpha;
//  drawInactiveColor.a = alpha;
//
//  renderer->FillRectDecal(screenPos, olc::vi2d(screenSize, screenSize),
//                          activated ? drawActiveColor : drawInactiveColor);
//  auto [p1, p2, p3] = GetTrianglePoints(screenPos, screenSize, facing);
//  renderer->FillTriangleDecal(p1, p2, p3,
//                              activated ? drawInactiveColor : drawActiveColor);
//}

void GridTile::SetFacing(Direction newFacing) {
  if (facing == newFacing) [[unlikely]]
    return;  // No change in facing
  facing = newFacing;

  // Store old directions
  // Exclude the input states. They are facing independent.
  // Why? Because a tile can never be rotated while the simulation is running.
  const auto [oldReceive, oldOutput] = [this] {
    return std::pair{canReceive, canOutput};
  }();

  // Rotate permissions by the difference amount
  for (auto& dir : AllDirections) {
    Direction newIndex = WorldToTileDirection(dir);
    // If this direction is invalid, throw.
    if (newIndex < Direction::Top || newIndex >= Direction::Count)
        [[unlikely]] {
      throw std::runtime_error("SetFacing: Invalid direction after rotation");
    }
    canReceive[newIndex] = oldReceive[dir];
    canOutput[newIndex] = oldOutput[dir];
  }
}

std::string GridTile::GetTileInformation() const {
  std::stringstream stream;
  // All in one line
  stream << "Tile Type: " << TileTypeToString(GetTileType()) << ", "
         << "Position: (" << pos.x << ", " << pos.y << "), "
         << "Facing: " << DirectionToString(facing) << ", "
         << "Activated Sides: [";
  for (const auto& dir : AllDirections) {
    if (inputStates[dir]) {
      // Convert world direction to tile-relative direction for display
      Direction tileRelativeDir = WorldToTileDirection(dir);
      stream << DirectionToString(tileRelativeDir) << " ";
    }
  }
  // Remove trailing space
  if (stream.tellp() > 0) [[likely]] {
    stream.seekp(-1, std::ios_base::end);
  }
  stream << "], "
         << "Activated: " << activated;
  return stream.str();
}

Direction GridTile::WorldToTileDirection(Direction worldDir) const {
  // Convert world direction to tile-relative by rotating left by facing.
  return DirectionRotate(worldDir, -static_cast<int>(facing));
}

Direction GridTile::TileToWorldDirection(Direction tileDir) const {
  // Convert tile-relative direction to world direction by rotating right by
  // facing.
  return DirectionRotate(tileDir, facing);
}

std::array<char, GRIDTILE_BYTESIZE> GridTile::Serialize() {
  std::array<char, GRIDTILE_BYTESIZE> data{};
  *reinterpret_cast<int*>(data.data()) = static_cast<int>(GetTileType());
  *reinterpret_cast<Direction*>(data.data() + sizeof(int)) = facing;
  *reinterpret_cast<int*>(data.data() + sizeof(int) + sizeof(facing)) = pos.x;
  *reinterpret_cast<int*>(data.data() + sizeof(int) + sizeof(facing) +
                          sizeof(pos.x)) = pos.y;
  return data;
}

std::unique_ptr<GridTile> GridTile::Deserialize(
    std::array<char, GRIDTILE_BYTESIZE> data) {
  int id = *reinterpret_cast<int*>(data.data());
  Direction facing = *reinterpret_cast<Direction*>(data.data() + sizeof(id));
  int posX = *reinterpret_cast<int*>(data.data() + sizeof(id) + sizeof(facing));
  int posY = *reinterpret_cast<int*>(data.data() + sizeof(id) + sizeof(facing) +
                                     sizeof(posX));
  vi2d pos = {posX, posY};
  std::unique_ptr<GridTile> tile;
  switch (id) {
    case 0:
      tile = std::make_unique<WireGridTile>(pos, facing);
      break;
    case 1:
      tile = std::make_unique<JunctionGridTile>(pos, facing);
      break;
    case 2:
      tile = std::make_unique<EmitterGridTile>(pos, facing);
      break;
    case 3:
      tile = std::make_unique<SemiConductorGridTile>(pos, facing);
      break;
    case 4:
      tile = std::make_unique<ButtonGridTile>(pos, facing);
      break;
    case 5:
      tile = std::make_unique<InverterGridTile>(pos, facing);
      break;
    case 6:
      tile = std::make_unique<CrossingGridTile>(pos, facing);
      break;
    default:
      throw std::runtime_error("Unknown tile ID");
  }
  return tile;
}

void GridTile::ResetActivation() {
  activated = defaultActivation;
  // Reset all input states
  for (const auto& dir : AllDirections) {
    inputStates[dir] = false;
  }
}

}  // namespace ElecSim
