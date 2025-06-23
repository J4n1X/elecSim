#include "Drawables.h"

#include <algorithm>
#include <ranges>

namespace Engine {

BasicTileDrawable::BasicTileDrawable(
    std::shared_ptr<ElecSim::GridTile> initialPtr, sf::Color activeColor,
    sf::Color inactiveColor)
    : TileDrawable(std::move(initialPtr)),
      activeColor(activeColor),
      inactiveColor(inactiveColor) {
  const float origin = size / 2.f;

  setOrigin({origin, origin});  // Origin is in the center.
  setPosition(Engine::ToSfmlVector(tilePtr->GetPos() * size) - getOrigin());
  vArray = CreateTileVertexArray();
}

void BasicTileDrawable::UpdateVisualState() {
  if (tilePtr->GetActivation()) {
    for(size_t i = 0; i < vArray.getVertexCount(); ++i) {
      if(i > 2 && i < 6) {
        vArray[i].color = inactiveColor;  // Triangles 1 and 3 are inactive
      } else {
        vArray[i].color = activeColor;     // Triangle 2 is active
      }
    }
  } else {
    for(size_t i = 0; i < vArray.getVertexCount(); ++i) {
      if(i > 2 && i < 6) {
        vArray[i].color = activeColor;  // Triangles 1 and 3 are active
      } else {
        vArray[i].color = inactiveColor;     // Triangle 2 is inactive
      }
    }
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
  target.draw(vArray, states);
}


// Test Vertex Array for a tile
sf::VertexArray BasicTileDrawable::CreateTileVertexArray() {
  sf::VertexArray v(sf::PrimitiveType::Triangles, 9);
  // Triangle one
  v[0].position = sf::Vector2f(0.f, 0.f);
  v[1].position = sf::Vector2f(size / 2.f, 0.f);
  v[2].position = sf::Vector2f(0.f, size);
  v[0].color = inactiveColor;
  v[1].color = inactiveColor;
  v[2].color = inactiveColor;

  // Triangle two
  v[3].position = sf::Vector2f(size / 2.f, 0.f);
  v[4].position = sf::Vector2f(size, size);
  v[5].position = sf::Vector2f(0.f, size);
  v[3].color = activeColor;
  v[4].color = activeColor;
  v[5].color = activeColor;

  // Triangle three
  v[6].position = sf::Vector2f(size / 2.f, 0.f);
  v[7].position = sf::Vector2f(size, 0.f);
  v[8].position = sf::Vector2f(size, size);
  v[6].color = inactiveColor;
  v[7].color = inactiveColor;
  v[8].color = inactiveColor;
  return v;
}

// ------------------------------------
// CrossingTileDrawable Implementation
// ------------------------------------

void CrossingTileDrawable::Setup() {
  setOrigin({size / 2.f, size / 2.f});  // Origin is in the center.
  setPosition(Engine::ToSfmlVector(tilePtr->GetPos() * size) - getOrigin());

  baseSquare.setPosition({0.f, 0.f});
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
  throw std::runtime_error("Unknown tile type for drawable creation");
}

}  // namespace Engine