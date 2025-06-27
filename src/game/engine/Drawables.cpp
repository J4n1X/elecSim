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

TileDrawable::TileDrawable(
    std::shared_ptr<ElecSim::GridTile> _tilePtr,
    std::array<std::shared_ptr<sf::VertexArray>, 2> _vArrays)
    : tilePtr(std::move(_tilePtr)), vArrays(std::move(_vArrays)) {
  const float origin = size / 2.f;
  setOrigin({origin, origin});  // Origin is in the center.
  setPosition(Engine::ToSfmlVector(tilePtr->GetPos() * size) + getOrigin());
  setRotation(sf::degrees(static_cast<float>(tilePtr->GetFacing()) * 90.f));
}

std::vector<sf::Vertex> TileDrawable::GetVertexArray() const {
  std::vector<sf::Vertex> vertices;
  const auto& vArray = *vArrays[tilePtr->GetActivation() ? 1 : 0];
  vertices.reserve(vArray.getVertexCount());
  for (size_t i = 0; i < vArray.getVertexCount(); ++i) {
    vertices.emplace_back(getTransform().transformPoint(vArray[i].position),
                          vArray[i].color);
  }
  return vertices;
}

void TileDrawable::draw(sf::RenderTarget& target,
                        sf::RenderStates states) const {
  states.transform *= getTransform();
  if (tilePtr->GetActivation()) {
    target.draw(*vArrays[1], states);
  } else {
    target.draw(*vArrays[0], states);
  }
}

// TODO: There's probably a better way. But I want this to work right now.
std::unique_ptr<TileDrawable> CreateTileRenderable(
    std::shared_ptr<ElecSim::GridTile> tilePtr,
    const std::vector<std::shared_ptr<sf::VertexArray>>& vArrays) {
  // Always type * 2
  auto index = static_cast<size_t>(tilePtr->GetTileType()) << 1;

  if (index >= vArrays.size()) {
    throw std::runtime_error(
        "Invalid tile type or activation state for rendering.");
  }
  return std::make_unique<TileDrawable>(
      std::move(tilePtr), std::array{vArrays[index], vArrays[index + 1]});
}

sf::Transform GetTileTransform(
    std::shared_ptr<ElecSim::GridTile> const& tilePtr) {
  if (!tilePtr) {
    throw std::invalid_argument("Tile pointer cannot be null.");
  }
  // I am too lazy to do math. sf::Transformable does it for us.
  sf::Transformable transformable;
  const float origin = TileDrawable::DEFAULT_SIZE / 2.f;
  transformable.setOrigin({origin, origin});
  transformable.setPosition(Engine::ToSfmlVector(tilePtr->GetPos() *
                             TileDrawable::DEFAULT_SIZE) + transformable.getOrigin());
  transformable.rotate(sf::degrees(static_cast<float>(tilePtr->GetFacing()) * 90.f));
  sf::Transform transform = transformable.getTransform();
  return transform;
}
}  // namespace Engine