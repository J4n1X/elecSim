#include "Drawables.h"

#include <algorithm>
#include <ranges>

namespace Engine {

#if 0
template <typename T>
  requires std::derived_from<T, ElecSim::GridTile>
constexpr void CreateTileDrawable(
    vf2d worldPos, ElecSim::Direction facing) {
  auto tilePtr = std::make_shared<T>(worldPos, facing);
  // Decide by the tile type which drawable to create (needs to be constexpr)
  if constexpr (std::is_same_v<T, ElecSim::WireGridTile>) {
    return std::make_unique<BasicTileDrawable<ElecSim::WireGridTile>>(
        std::move(tilePtr), sf::Color(128, 128, 0), sf::Color(255, 255, 255));
  }
  if constexpr (std::is_same_v<T, ElecSim::JunctionGridTile>) {
    return std::make_unique<BasicTileDrawable<ElecSim::JunctionGridTile>>(
        std::move(tilePtr), sf::Color(255, 255, 0), sf::Color(192, 192, 192));
  }
  if constexpr (std::is_same_v<T, ElecSim::EmitterGridTile>) {
    return std::make_unique<BasicTileDrawable<ElecSim::EmitterGridTile>>(
        std::move(tilePtr), sf::Color(0, 255, 255), sf::Color(0, 128, 128));
  }
  if constexpr (std::is_same_v<T, ElecSim::SemiConductorGridTile>) {
    return std::make_unique<BasicTileDrawable<ElecSim::SemiConductorGridTile>>(
        std::move(tilePtr), sf::Color(0, 255, 0), sf::Color(0, 128, 0));
  }
  if constexpr (std::is_same_v<T, ElecSim::ButtonGridTile>) {
    return std::make_unique<BasicTileDrawable<ElecSim::ButtonGridTile>>(
        std::move(tilePtr), sf::Color(255, 0, 0), sf::Color(128, 0, 0));
  }
  if constexpr (std::is_same_v<T, ElecSim::InverterGridTile>) {
    return std::make_unique<BasicTileDrawable<ElecSim::InverterGridTile>>(
        std::move(tilePtr), sf::Color(255, 0, 255), sf::Color(128, 0, 128));
  }
  if constexpr (std::is_same_v<T, ElecSim::CrossingGridTile>) {
    return std::make_unique<BasicTileDrawable<ElecSim::CrossingGridTile>>(
        std::move(tilePtr), sf::Color(0, 0, 255), sf::Color(0, 0, 128));
  }
}

void CrossingDrawable::Draw(olc::PixelGameEngine *renderer, vf2d pos,
                            float screenSize, uint8_t alpha) const {
  // Get colors based on activation state and alpha
  olc::Pixel drawActiveColor = activeColors[tilePtr->GetTileTypeId()];
  olc::Pixel drawInactiveColor = inactiveColors[tilePtr->GetTileTypeId()];
  drawActiveColor.a = alpha;
  drawInactiveColor.a = alpha;

  // Draw the base square
  renderer->FillRectDecal(olc::vf2d(pos.x, pos.y),
                          olc::vi2d(screenSize, screenSize), drawInactiveColor);

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
#endif

BasicTileDrawable::BasicTileDrawable(
    std::shared_ptr<ElecSim::GridTile> initialPtr, sf::Color activeColor,
    sf::Color inactiveColor)
    : TileDrawable(std::move(initialPtr)),
      activeColor(activeColor),
      inactiveColor(inactiveColor) {
  const float origin = size / 2.f;

  setOrigin({origin, origin});  // Origin is in the center.
  setPosition(Engine::ToSfmlVector(tilePtr->GetPos() * size) - getOrigin());

  // Because we have changed the origin, we need to adjust the position 
  // of the shapes accordingly.
  square.setPosition({0.f, 0.f});
  triangle.setPosition({0.f, 0.f});

  square.setSize({size, size});

  triangle.setPointCount(3);
  triangle.setPoint(0, {size / 2.f, 0.f});
  triangle.setPoint(1, {0.f, size});
  triangle.setPoint(2, {size, size});
}

void BasicTileDrawable::UpdateVisualState() {
  if (tilePtr->GetActivation()) {
    square.setFillColor(activeColor);
    triangle.setFillColor(inactiveColor);
  } else {
    square.setFillColor(inactiveColor);
    triangle.setFillColor(activeColor);
  }
  // Rotation based on the facing
  switch (tilePtr->GetFacing()) {
    case ElecSim::Direction::Top:
      setRotation(sf::degrees(0.f));
      break;
    case ElecSim::Direction::Right:
      setRotation(sf::degrees(90.f));
      break;
    case ElecSim::Direction::Bottom:
      setRotation(sf::degrees(180.f));
      break;
    case ElecSim::Direction::Left:
      setRotation(sf::degrees(270.f));
      break;
    default:
      throw std::runtime_error("Invalid tile facing direction");
  }
}

[[nodiscard]] std::unique_ptr<TileDrawable> BasicTileDrawable::Clone() const {
  return std::make_unique<BasicTileDrawable>(tilePtr, activeColor,
                                             inactiveColor);
}

void BasicTileDrawable::draw(sf::RenderTarget& target,
                             sf::RenderStates states) const {
  // Apply the current transform to the states
  states.transform *= getTransform();

  // Draw the square and triangle
  target.draw(square, states);
  target.draw(triangle, states);
}

// ------------------------------------
// CrossingTileDrawable Implementation
// ------------------------------------

void CrossingTileDrawable::Setup() {
  setPosition(Engine::ToSfmlVector(tilePtr->GetPos() * size));
  setOrigin({size / 2.f, size / 2.f});  // Origin is in the center.

  baseSquare.setSize({size, size});

  const float trenchSize = size / 8.f;
  const float squareSize = (size - trenchSize) / 2.f;
  const float offset = squareSize + trenchSize;
  for (auto& square : squares) {
    square.setSize({squareSize, squareSize});
  }
  squares[0].setPosition({0.f, 0.f});
  squares[1].setPosition({offset, 0.f});
  squares[2].setPosition({0.f, offset});
  squares[3].setPosition({offset, offset});
}

CrossingTileDrawable::CrossingTileDrawable(
    std::shared_ptr<ElecSim::CrossingGridTile> initialPtr)
    : TileDrawable(std::move(initialPtr)) {
  Setup();
};

CrossingTileDrawable::CrossingTileDrawable(
    std::shared_ptr<ElecSim::GridTile> initialPtr)
    : TileDrawable(std::move(initialPtr)) {
  if (tilePtr->GetTileType() != ElecSim::TileType::Crossing) {
    throw std::runtime_error("Invalid tile type for CrossingTileDrawable");
  } else {
    Setup();
  }
}

[[nodiscard]] std::unique_ptr<TileDrawable> CrossingTileDrawable::Clone()
    const {
  return std::make_unique<CrossingTileDrawable>(tilePtr);
}

void CrossingTileDrawable::UpdateVisualState() {
  if (tilePtr->GetActivation()) {
    baseSquare.setFillColor(activeColor);
    for (auto& square : squares) {
      square.setFillColor(activeColor);
    }
  } else {
    baseSquare.setFillColor(inactiveColor);
    for (auto& square : squares) {
      square.setFillColor(inactiveColor);
    }
  }
}

void CrossingTileDrawable::draw(sf::RenderTarget& target,
                                sf::RenderStates states) const {
  states.transform *= getTransform();
  for (const auto& square : squares) {
    target.draw(square, states);
  }
}

// TODO: There's probably a better way. But I want this to work right now.
std::unique_ptr<TileDrawable> CreateTileDrawable(
    std::shared_ptr<ElecSim::GridTile> tilePtr) {
  switch (tilePtr->GetTileType()) {
    using ElecSim::TileType;
    case TileType::Wire:
      return std::make_unique<BasicTileDrawable>(
          std::move(tilePtr), sf::Color(128, 128, 0), sf::Color(255, 255, 255));
    case TileType::Junction:
      return std::make_unique<BasicTileDrawable>(
          std::move(tilePtr), sf::Color(255, 255, 0), sf::Color(192, 192, 192));
    case TileType::Emitter:
      return std::make_unique<BasicTileDrawable>(
          std::move(tilePtr), sf::Color(0, 255, 255), sf::Color(0, 128, 128));
    case TileType::SemiConductor:
      return std::make_unique<BasicTileDrawable>(
          std::move(tilePtr), sf::Color(0, 255, 0), sf::Color(0, 128, 0));
    case TileType::Button:
      return std::make_unique<BasicTileDrawable>(
          std::move(tilePtr), sf::Color(255, 0, 0), sf::Color(128, 0, 0));
    case TileType::Inverter:
      return std::make_unique<BasicTileDrawable>(
          std::move(tilePtr), sf::Color(255, 0, 255), sf::Color(128, 0, 128));
    case TileType::Crossing:
      return std::make_unique<CrossingTileDrawable>(std::move(tilePtr));
  }
}

}  // namespace Engine