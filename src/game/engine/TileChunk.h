#pragma once

#include "GridTile.h"
#include "SFML/Graphics.hpp"

namespace Engine {

/**
 * @class TileChunk
 * @brief A chunk of tiles that are rendered together for performance optimization.
 * 
 * TileChunk represents a fixed-size grid of tiles (64x64) that are batched together into a single
 * vertex array for efficient rendering. It inherits from sf::Drawable to be directly renderable
 * and from sf::Transformable to support transformation operations.
 * 
 * Each tile in the chunk occupies 1 unit in world space (TILE_WORLD_SIZE = 1.0f), and the entire
 * chunk has a position in world coordinates.
 * 
 * The chunk uses a texture atlas for rendering tiles, with individual texture rectangles
 * specified per tile.
 */
class TileChunk : public sf::Drawable, public sf::Transformable {
 public:
  TileChunk() = default;
  TileChunk(sf::Vector2f worldPos, const sf::Texture* atlas = nullptr);

  /// @brief Set a tile in the chunk, using a certain part of the texture atlas.
  /// @param tile The tile to add
  /// @param textureRect The rectangle which defines how the texture is mapped.
  void SetTile(const ElecSim::GridTile* tile, sf::IntRect textureRect);
  
  /// @brief Removes a tile at the given position.
  /// @param tilePos The position of the tile to remove.
  void EraseTile(const ElecSim::vi2d& tilePos);

  /// @brief Set the chunk texture atlas.
  /// @param atlas The texture atlas to set.
  inline void SetTexture(const sf::Texture* atlas) noexcept { texture = atlas; }

  constexpr static float TILE_WORLD_SIZE = 1.f;
  constexpr static size_t CHUNK_LENGTH = 64;  // Side length in tiles
 private:
  // TODO: Make the rest of the code use this instead of TileDrawable::TILE_SIZE
  constexpr static size_t CHUNK_TILE_COUNT = CHUNK_LENGTH * CHUNK_LENGTH;
  constexpr static size_t VERTICES_PER_TILE = 6;  // 2 triangles per tile

  const sf::Texture* texture = nullptr;
  sf::VertexArray vArray{sf::PrimitiveType::Triangles,
                         CHUNK_TILE_COUNT* VERTICES_PER_TILE};
  void draw(sf::RenderTarget& target,
            sf::RenderStates states) const final override;
};
}  // namespace Engine