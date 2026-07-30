#pragma once

#include <bitset>
#include <cstdint>
#include <memory>
#include <vector>

#include "GridTile.h"
#include "SFML/Graphics.hpp"

namespace Engine {

/**
 * @class TileChunk
 * @brief A chunk of tiles batched into a single GPU-resident vertex buffer.
 *
 * The chunk keeps an authoritative CPU-side vertex array and mirrors it into an
 * sf::VertexBuffer held in graphics memory. Only tiles that actually changed are
 * re-uploaded, and the upload happens lazily on the first draw after a change --
 * so a chunk that is modified while off-screen costs nothing until it becomes
 * visible.
 *
 * If sf::VertexBuffer is unavailable on the running system, drawing falls back
 * to streaming the CPU array directly, which is exactly the old behaviour.
 */
class TileChunk : public sf::Drawable, public sf::Transformable {
 public:
  TileChunk() = default;
  TileChunk(sf::Vector2f worldPos, const sf::Texture* atlas = nullptr);

  /// @brief Set a tile in the chunk, using a certain part of the texture atlas.
  void SetTile(const ElecSim::GridTile* tile, sf::IntRect textureRect);

  /// @brief Removes a tile at the given position.
  void EraseTile(const ElecSim::vi2d& tilePos);

  /// @brief Set the chunk texture atlas.
  inline void SetTexture(const sf::Texture* atlas) noexcept { texture = atlas; }

  constexpr static float TILE_WORLD_SIZE = 1.f;
  constexpr static std::size_t CHUNK_LENGTH = 64;  // Side length in tiles

 private:
  constexpr static std::size_t CHUNK_TILE_COUNT = CHUNK_LENGTH * CHUNK_LENGTH;
  constexpr static std::size_t VERTICES_PER_TILE = 6;  // 2 triangles per tile
  constexpr static std::size_t VERTEX_COUNT =
      CHUNK_TILE_COUNT * VERTICES_PER_TILE;

  static_assert(CHUNK_TILE_COUNT <= UINT16_MAX + 1,
                "Dirty slot indices are stored as uint16_t.");

  const sf::Texture* texture = nullptr;

  /// Authoritative CPU-side copy. Always valid, always up to date.
  std::vector<sf::Vertex> vertices = std::vector<sf::Vertex>(VERTEX_COUNT);

  /// GPU mirror. Held by pointer for two reasons:
  ///   1. sf::VertexBuffer declares a copy ctor and a destructor, so it has no
  ///      move ctor. Stored by value, a chunk map rehash would deep-copy every
  ///      GPU buffer in the map.
  ///   2. It lets the buffer be created lazily, on first draw.
  /// Mutable because draw() is const but is where the sync happens.
  mutable std::unique_ptr<sf::VertexBuffer> buffer;

  /// Tile slots changed since the last upload, plus a membership bitset so the
  /// list stays deduplicated without a hash set.
  mutable std::vector<std::uint16_t> dirtySlots;
  mutable std::bitset<CHUNK_TILE_COUNT> slotIsDirty;

  /// A marker of the highest tile slot written to. This is used so 
  /// that the vertex buffer draw can be partial. 
  std::size_t highWaterMark = 0;

  /// Local tile coordinates within the chunk, correct for negative world
  /// positions.
  [[nodiscard]] static sf::Vector2i LocalCoords(
      const ElecSim::vi2d& tilePos) noexcept;
  [[nodiscard]] static std::size_t SlotIndex(sf::Vector2i local) noexcept;

  void MarkDirty(std::size_t slot) const noexcept;
  void ClearDirty() const noexcept;

  /// Push pending changes to the GPU. No-op when nothing changed.
  void Sync() const;

  void draw(sf::RenderTarget& target,
            sf::RenderStates states) const final override;
};

}  // namespace Engine