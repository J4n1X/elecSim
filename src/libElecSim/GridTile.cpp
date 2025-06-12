#include "GridTile.h"

#include <cmath>

#include "GridTileTypes.h"

namespace ElecSim {

// --- GridTile Implementation ---

// Helper function to get the points of the triangle for a tile
std::array<olc::vf2d, 3> GridTile::GetTrianglePoints(olc::vf2d screenPos,
                                                     int screenSize,
                                                     Direction facing) {
  std::array<olc::vf2d, 3> points;

  // Calculate the points based on facing direction
  switch (facing) {
    case Direction::Top:
      points[0] =
          olc::vf2d(screenPos.x + screenSize / 2, screenPos.y);  // Top middle
      points[1] = olc::vf2d(screenPos.x + screenSize,
                            screenPos.y + screenSize);  // Bottom right
      points[2] =
          olc::vf2d(screenPos.x, screenPos.y + screenSize);  // Bottom left
      break;
    case Direction::Right:
      points[0] = olc::vf2d(screenPos.x + screenSize,
                            screenPos.y + screenSize / 2);  // Right middle
      points[1] =
          olc::vf2d(screenPos.x, screenPos.y + screenSize);  // Bottom left
      points[2] = olc::vf2d(screenPos.x, screenPos.y);       // Top left
      break;
    case Direction::Bottom:
      points[0] = olc::vf2d(screenPos.x + screenSize / 2,
                            screenPos.y + screenSize);  // Bottom middle
      points[1] = olc::vf2d(screenPos.x, screenPos.y);  // Top left
      points[2] =
          olc::vf2d(screenPos.x + screenSize, screenPos.y);  // Top right
      break;
    case Direction::Left:
      points[0] =
          olc::vf2d(screenPos.x, screenPos.y + screenSize / 2);  // Left middle
      points[1] =
          olc::vf2d(screenPos.x + screenSize, screenPos.y);  // Top right
      points[2] = olc::vf2d(screenPos.x + screenSize,
                            screenPos.y + screenSize);  // Bottom right
      break;
    default:
      // Fallback to upward facing triangle
      points[0] = olc::vf2d(screenPos.x + screenSize / 2, screenPos.y);
      points[1] = olc::vf2d(screenPos.x + screenSize, screenPos.y + screenSize);
      points[2] = olc::vf2d(screenPos.x, screenPos.y + screenSize);
  }
  return points;
}

// --- GridTile Implementation ---

GridTile::GridTile(olc::vi2d pos, Direction facing, float size,
                   bool defaultActivation, olc::Pixel inactiveColor,
                   olc::Pixel activeColor)
    : pos(pos),
      facing(facing),
      size(size),
      activated(defaultActivation),
      defaultActivation(defaultActivation),
      inactiveColor(inactiveColor),
      activeColor(activeColor),
      canReceive{},
      canOutput{},
      inputStates{} {
  // Initialize I/O arrays
}

void GridTile::Draw(olc::PixelGameEngine* renderer, olc::vf2d screenPos,
                    float screenSize, int alpha) {
  // Get colors based on activation state and alpha
  activeColor.a = alpha;
  inactiveColor.a = alpha;

  renderer->FillRectDecal(screenPos, olc::vi2d(screenSize, screenSize),
                          activated ? activeColor : inactiveColor);
  auto [p1, p2, p3] = GetTrianglePoints(screenPos, screenSize, facing);
  renderer->FillTriangleDecal(p1, p2, p3,
                              activated ? inactiveColor : activeColor);
}

void GridTile::SetFacing(Direction newFacing) {
  if (facing == newFacing) return;  // No change in facing
  facing = newFacing;

  // Store old directions
  // Exclude the input states. They are facing independent.
  // Why? Because a tile can never be rotated while the simulation is running.
  TileSideStates oldReceive;
  TileSideStates oldOutput;
  std::copy(std::begin(canReceive), std::end(canReceive),
            std::begin(oldReceive));
  std::copy(std::begin(canOutput), std::end(canOutput), std::begin(oldOutput));

  // Rotate permissions by the difference amount
  for (auto& dir : AllDirections) {
    Direction newIndex = WorldToTileDirection(dir);
    canReceive[newIndex] = oldReceive[dir];
    canOutput[newIndex] = oldOutput[dir];
  }
}

std::string GridTile::GetTileInformation()
    const {  // FIXED: Removed the "janky" logic that was needed due to
             // coordinate system confusion.
  // The inputStates array is indexed by world coordinates, but for display we
  // want tile-relative coordinates. Now using proper WorldToTileDirection
  // conversion.
  std::stringstream stream;
  // All in one line
  stream << "Tile Type: " << TileTypeName() << ", "
         << "Position: (" << pos.x << ", " << pos.y << "), "
         << "Facing: " << DirectionToString(facing) << ", "
         << "Size: " << size << ", "
         << "Activated Sides: [";
  for (const auto& dir : AllDirections) {
    if (inputStates[dir]) {
      // Convert world direction to tile-relative direction for display
      Direction tileRelativeDir = WorldToTileDirection(dir);
      stream << DirectionToString(tileRelativeDir) << " ";
    }
  }
  // Remove trailing space
  if (stream.str().back() == ' ') {
    stream.seekp(-1, std::ios_base::end);
  }
  stream << "], "
         << "Activated: " << activated;  return stream.str();
}

Direction GridTile::WorldToTileDirection(Direction worldDir) const {
  // Convert world direction to tile-relative by rotating backwards by facing amount
  return DirectionRotate(worldDir, -static_cast<int>(facing));
}

Direction GridTile::TileToWorldDirection(Direction tileDir) const {
  // Convert tile-relative direction to world direction by rotating by facing amount
  return DirectionRotate(tileDir, facing);
}

std::array<char, GRIDTILE_BYTESIZE> GridTile::Serialize() {
  std::array<char, GRIDTILE_BYTESIZE> data{};
  *reinterpret_cast<int*>(data.data()) = GetTileTypeId();
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
  olc::vi2d pos = {posX, posY};
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
