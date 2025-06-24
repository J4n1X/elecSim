#include "Drawables.h"

#include <algorithm>
#include <ranges>

// Very basic helper struct
struct Triangle {
  constexpr static uint32_t VERTEX_COUNT = 3;
  std::array<sf::Vector2f, 3> positions;
  sf::Color color;
};

namespace Engine {

BasicTileDrawable::BasicTileDrawable(
    std::shared_ptr<ElecSim::GridTile> initialPtr, sf::Color activeColor,
    sf::Color inactiveColor)
    : TileDrawable(std::move(initialPtr)),
      activeColor(activeColor),
      inactiveColor(inactiveColor) {
  const float origin = size / 2.f;

  setOrigin({origin, origin});  // Origin is in the center.
  setPosition(Engine::ToSfmlVector(tilePtr->GetPos() * size) + getOrigin());

  vArray = CreateVertexArray();
}

void BasicTileDrawable::UpdateVisualState() {
  if (tilePtr->GetActivation()) {
    for (size_t i = 0; i < vArray.getVertexCount(); ++i) {
      if (i > 2 && i < 6) {
        vArray[i].color = inactiveColor;  // Triangles 1 and 3 are inactive
      } else {
        vArray[i].color = activeColor;  // Triangle 2 is active
      }
    }
  } else {
    for (size_t i = 0; i < vArray.getVertexCount(); ++i) {
      if (i > 2 && i < 6) {
        vArray[i].color = activeColor;  // Triangles 1 and 3 are active
      } else {
        vArray[i].color = inactiveColor;  // Triangle 2 is inactive
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
sf::VertexArray BasicTileDrawable::CreateVertexArray() const {
  sf::VertexArray v(sf::PrimitiveType::Triangles, 9);

  const std::array<Triangle, 3> triangles = {
      {// Triangle one (inactive by default)
       {{sf::Vector2f(0.f, 0.f), sf::Vector2f(size / 2.f, 0.f),
         sf::Vector2f(0.f, size)},
        inactiveColor},
       // Triangle two (active by default)
       {{sf::Vector2f(size / 2.f, 0.f), sf::Vector2f(size, size),
         sf::Vector2f(0.f, size)},
        activeColor},
       // Triangle three (inactive by default)
       {{sf::Vector2f(size / 2.f, 0.f), sf::Vector2f(size, 0.f),
         sf::Vector2f(size, size)},
        inactiveColor}}};

  for (size_t t = 0; t < triangles.size(); ++t) {
    for (size_t i = 0; i < 3; ++i) {
      // auto transformedPos = getTransform().transformPoint(
      //     triangles[t].positions[i]);
      // v[t * 3 + i] = sf::Vertex(
      //     transformedPos, triangles[t].color);
      v[t * 3 + i] = sf::Vertex(triangles[t].positions[i], triangles[t].color);
    }
  }
  return v;
}

// ------------------------------------
// CrossingTileDrawable Implementation
// ------------------------------------

void CrossingTileDrawable::Setup() {
  setOrigin({size / 2.f, size / 2.f});  // Origin is in the center.
  setPosition(Engine::ToSfmlVector(tilePtr->GetPos() * size) + getOrigin());
  vArray = CreateVertexArray();
}

sf::VertexArray CrossingTileDrawable::CreateVertexArray() const {
  const float trenchSize = size / 8.f;
  const float squareSize = (size - trenchSize) / 2.f;
  const float offset = squareSize + trenchSize;
  sf::VertexArray v(sf::PrimitiveType::Triangles, Triangle::VERTEX_COUNT * 8);

  // Lambda that creates a quad
  auto createQuad = [](sf::Vector2f pos, sf::Color color,
                       float size) -> std::array<Triangle, 2> {
    return {
        Triangle{
            {pos, pos + sf::Vector2f(size, 0.f), pos + sf::Vector2f(0.f, size)},
            color},  // Triangle 1
        Triangle{{pos + sf::Vector2f(size, 0.f), pos + sf::Vector2f(size, size),
                  pos + sf::Vector2f(0.f, size)},
                 color}  // Triangle 2
    };
  };
  for (size_t i = 0; i < 4; i++) {
    float offsetX = (i % 2) * offset;
    float offsetY = (i / 2) * offset;
    auto triangles = createQuad({offsetX, offsetY}, color, squareSize);
    size_t baseIndex = i * Triangle::VERTEX_COUNT * 2;
    auto vertices =
        triangles | std::views::transform(
                        [](const Triangle& t) -> std::array<sf::Vertex, 3> {
                          return {sf::Vertex(t.positions[0], t.color),
                                  sf::Vertex(t.positions[1], t.color),
                                  sf::Vertex(t.positions[2], t.color)};
                        }) | std::views::join;
    // Flatten the triangles into vertices and assign to v using ranges and views
    size_t idx = 0;
    for (const auto& vert : vertices) {
      v[baseIndex + idx++] = vert;
    }
  }
  return v;
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
  // Do nothing
  return;
}

void CrossingTileDrawable::draw(sf::RenderTarget& target,
                                sf::RenderStates states) const {
  states.transform *= getTransform();
  target.draw(vArray, states);
}

// TODO: There's probably a better way. But I want this to work right now.
std::unique_ptr<TileDrawable> CreateTileRenderable(
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