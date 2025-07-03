#include "Drawables.h"

#include <algorithm>
#include <ranges>

#include "ArrowTile_mesh_blob.h"
#include "CrossingTile_mesh_blob.h"

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

sf::Transform GetTileTransform(const ElecSim::GridTile* tile) {
  // I am too lazy to do math. sf::Transformable does it for us.
  sf::Transformable transformable;
  const float origin = TileDrawable::DEFAULT_SIZE / 2.f;
  transformable.setOrigin({origin, origin});
  transformable.setPosition(
      Engine::ToSfmlVector(tile->GetPos() * TileDrawable::DEFAULT_SIZE) +
      transformable.getOrigin());
  transformable.rotate(
      sf::degrees(static_cast<float>(tile->GetFacing()) * 90.f));
  sf::Transform transform = transformable.getTransform();
  return transform;
}

TileTextureAtlas::TileTextureAtlas(uint32_t initTilePixelSize)
    : tilePixelSize(initTilePixelSize) {
  meshes.reserve(ElecSim::GRIDTILE_COUNT * 2);
  MeshLoader loader;
  loader.LoadMeshFromString(std::string(
      reinterpret_cast<const char*>(ArrowTile_mesh_data), ArrowTile_mesh_len));
  loader.LoadMeshFromString(
      std::string(reinterpret_cast<const char*>(CrossingTile_mesh_data),
                  CrossingTile_mesh_len));
  for (size_t i = 0; i < ElecSim::GRIDTILE_COUNT; ++i) {
    auto inactiveMesh = loader.GetMesh(i * 2);
    auto activeMesh = loader.GetMesh(i * 2 + 1);
    if (!inactiveMesh || !activeMesh) [[unlikely]] {
      throw std::runtime_error("Failed to load tile meshes.");
    }
    meshes.push_back(
        TileMesh{*inactiveMesh, *activeMesh});  // Copy the vertex arrays
  }
  UpdateTextureAtlas();
}

sf::IntRect TileTextureAtlas::GetTileRect(ElecSim::TileType type,
                                          bool activation) const {
  int typeInt = static_cast<int>(type);
  int tilePixelSizeInt = static_cast<int>(tilePixelSize);
  int xOffset = typeInt * tilePixelSizeInt;
  int yOffset = activation ? tilePixelSizeInt : 0;
  const sf::Vector2i offset{xOffset, yOffset};
  const sf::Vector2i rectSize(tilePixelSizeInt, tilePixelSizeInt);
  return sf::IntRect(offset, rectSize);
}

sf::IntRect TileTextureAtlas::GetDefaultTileRect(ElecSim::TileType type,
                                               bool activation) {
  constexpr int DEFAULT_TILE_SIZE = 32; // Default pixel size for tiles
  int typeInt = static_cast<int>(type);
  int xOffset = typeInt * DEFAULT_TILE_SIZE;
  int yOffset = activation ? DEFAULT_TILE_SIZE : 0;
  const sf::Vector2i offset{xOffset, yOffset};
  const sf::Vector2i rectSize(DEFAULT_TILE_SIZE, DEFAULT_TILE_SIZE);
  return sf::IntRect(offset, rectSize);
}

void TileTextureAtlas::UpdateTextureAtlas() {
  renderTarget.clear();
  // We can downcast without worry here. If we really store 4 billion meshes,
  // the computer's probably on fire anyway.
  uint32_t atlasLength = tilePixelSize * static_cast<uint32_t>(meshes.size());
  // And yes, this texture is just a very long strip. Inactive on top, active
  // on bottom.
  if (!renderTarget.resize(sf::Vector2u(atlasLength, tilePixelSize * 2))) {
    throw std::runtime_error("Failed to resize render target for tile atlas.");
  }
  const float tileSizeF = static_cast<float>(tilePixelSize);
  const float requiredScale = tileSizeF / TileDrawable::DEFAULT_SIZE;
  const sf::Vector2f scaleVector{requiredScale, requiredScale};

  for (const auto& [index, mesh] : meshes | std::views::enumerate) {
    const float xOffset = static_cast<float>(index) * tileSizeF;

    // Draw inactive mesh at top half
    sf::Transform inactiveTransform;
    inactiveTransform.translate({xOffset, 0.f}).scale(scaleVector);
    renderTarget.draw(mesh.inactiveMesh, inactiveTransform);

    // Draw active mesh at bottom half
    sf::Transform activeTransform;
    activeTransform.translate({xOffset, tileSizeF}).scale(scaleVector);
    renderTarget.draw(mesh.activeMesh, activeTransform);
  }
  renderTarget.display();
  if(!renderTarget.generateMipmap()) {
    std::cerr << "Failed to generate mipmaps for tile atlas texture."
              << std::endl;
  }
}

}  // namespace Engine