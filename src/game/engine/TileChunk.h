#pragma once

#include "GridTile.h"
#include "SFML/Graphics.hpp"

namespace Engine {

class TileChunk : public sf::Drawable, public sf::Transformable {
 public:
  TileChunk() = default;
  TileChunk(sf::Vector2f worldPos, const sf::Texture* atlas = nullptr);
  void SetTile(const ElecSim::GridTile* tile, sf::IntRect textureRect);
  void EraseTile(const ElecSim::vi2d& tilePos);
  void SetTexture(const sf::Texture* atlas) { texture = atlas; }

  constexpr static float TILE_WORLD_SIZE = 1.f;
  constexpr static size_t CHUNK_LENGTH = 64;  // Side length in tiles
 private:
  // TODO: Make the rest of the code use this instead of TileDrawable::TILE_SIZE
  constexpr static size_t CHUNK_TILE_COUNT = CHUNK_LENGTH * CHUNK_LENGTH;
  constexpr static size_t VERTICES_PER_TILE = 6;  // 2 triangles per tile

  const sf::Texture* texture = nullptr;
  sf::VertexArray vArray{sf::PrimitiveType::Triangles,
                         CHUNK_TILE_COUNT* VERTICES_PER_TILE};
  void draw(sf::RenderTarget& target, sf::RenderStates states) const final override;
};
}  // namespace Engine