#include "Drawables.h"
namespace Engine {
namespace Target {
namespace Olc {

const std::array<olc::Pixel, 7> activeColors = {
    olc::DARK_YELLOW, olc::YELLOW,  olc::CYAN, olc::GREEN,
    olc::RED,         olc::MAGENTA, olc::BLUE};

const std::array<olc::Pixel, 7> inactiveColors = {
    olc::WHITE,    olc::GREY,         olc::DARK_CYAN, olc::DARK_GREEN,
    olc::DARK_RED, olc::DARK_MAGENTA, olc::DARK_BLUE};

constexpr std::array<olc::vf2d, 3> GetTrianglePoints(
    olc::vf2d pos, int screenSize, ElecSim::Direction facing) {
  std::array<olc::vf2d, 3> points;
  using ElecSim::Direction;

  // Calculate the points based on facing direction
  switch (facing) {
    case Direction::Top:
      points[0] = olc::vf2d(pos.x + screenSize / 2, pos.y);  // Top middle
      points[1] = olc::vf2d(pos.x + screenSize,
                            pos.y + screenSize);         // Bottom right
      points[2] = olc::vf2d(pos.x, pos.y + screenSize);  // Bottom left
      break;
    case Direction::Right:
      points[0] = olc::vf2d(pos.x + screenSize,
                            pos.y + screenSize / 2);     // Right middle
      points[1] = olc::vf2d(pos.x, pos.y + screenSize);  // Bottom left
      points[2] = olc::vf2d(pos.x, pos.y);               // Top left
      break;
    case Direction::Bottom:
      points[0] = olc::vf2d(pos.x + screenSize / 2,
                            pos.y + screenSize);         // Bottom middle
      points[1] = olc::vf2d(pos.x, pos.y);               // Top left
      points[2] = olc::vf2d(pos.x + screenSize, pos.y);  // Top right
      break;
    case Direction::Left:
      points[0] = olc::vf2d(pos.x, pos.y + screenSize / 2);  // Left middle
      points[1] = olc::vf2d(pos.x + screenSize, pos.y);      // Top right
      points[2] = olc::vf2d(pos.x + screenSize,
                            pos.y + screenSize);  // Bottom right
      break;
    default:
      // Fallback to upward facing triangle
      points[0] = olc::vf2d(pos.x + screenSize / 2, pos.y);
      points[1] = olc::vf2d(pos.x + screenSize, pos.y + screenSize);
      points[2] = olc::vf2d(pos.x, pos.y + screenSize);
  }
  return points;
}
void OlcDrawTile(olc::PixelGameEngine *renderer, olc::vf2d pos,
                        float size, int alpha, bool activation,
                        ElecSim::Direction facing, int tileId) {
  olc::Pixel drawActiveColor = activeColors[tileId];
  olc::Pixel drawInactiveColor = inactiveColors[tileId];
  drawActiveColor.a = alpha;
  drawInactiveColor.a = alpha;

  renderer->FillRectDecal(pos, olc::vi2d(size, size),
                          activation ? drawActiveColor : drawInactiveColor);
  auto [p1, p2, p3] = GetTrianglePoints(pos, size, facing);
  renderer->FillTriangleDecal(p1, p2, p3,
                              activation ? drawInactiveColor : drawActiveColor);
}

void CrossingDrawable::Draw(olc::PixelGameEngine *renderer, vf2d pos,
                            float screenSize, uint8_t alpha) const {
  // Get colors based on activation state and alpha
  olc::Pixel drawActiveColor = activeColors[tilePtr->GetTileTypeId()];
  olc::Pixel drawInactiveColor = inactiveColors[tilePtr->GetTileTypeId()];
  drawActiveColor.a = alpha;
  drawInactiveColor.a = alpha;

  // Draw the base square
  renderer->FillRectDecal(olc::vf2d(pos.x, pos.y), olc::vi2d(screenSize, screenSize),
                          drawInactiveColor);

  // Draw crossing lines that touch the edges
  float lineThickness = screenSize / 10.0f;  // Adjust thickness as needed

  // Horizontal line - top left to bottom right
  renderer->FillRectDecal(
      olc::vf2d(pos.x, pos.y + (screenSize - lineThickness) / 2),
      olc::vf2d(screenSize, lineThickness), drawActiveColor);

  // Vertical line - top to bottom
  renderer->FillRectDecal(
      olc::vf2d(pos.x + (screenSize - lineThickness) / 2, pos.y),
      olc::vf2d(lineThickness, screenSize), drawActiveColor);
}

}  // namespace Olc
}  // namespace Target
}  // namespace Engine