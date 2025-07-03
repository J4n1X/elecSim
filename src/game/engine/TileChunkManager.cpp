#include "TileChunkManager.h"
#include "Drawables.h"
#include "Common.h"

namespace Engine {
// TODO: Unify the chunk management API across the codebase. Current API has several 
// inconsistencies in parameter types and lacks a cohesive design pattern.
// Consider creating a unified interface that handles both grid and visual representation.

void TileChunkManager::SetTile(const ElecSim::GridTile* tile,
                               sf::IntRect textureRect) {
  // TODO: Extract this alignment lambda to a common utility function or method
  // that can be reused across SetTile, EraseTile and other methods.
  const auto alignToChunkGrid = [](int pos) constexpr noexcept -> int {
    const auto [quot, rem] =
        std::div(pos, static_cast<int>(TileChunk::CHUNK_LENGTH));
    return (quot - (rem < 0)) * static_cast<int>(TileChunk::CHUNK_LENGTH);
  };
  const auto& tilePos = tile->GetPos();
  const auto chunkBasePos =
      ElecSim::vi2d(alignToChunkGrid(tilePos.x), alignToChunkGrid(tilePos.y));

  if (auto it = chunks.find(chunkBasePos); it != chunks.end()) {
    it->second.SetTile(tile, textureRect);
  } else {  // Construct new chunk if it doesn't exist yet.
    // Use the texture from the texture rect
    TileChunk newChunk(sf::Vector2f(chunkBasePos.x, chunkBasePos.y));
    newChunk.SetTile(tile, textureRect);
    chunks.emplace(chunkBasePos, std::move(newChunk));
  }
}

void TileChunkManager::SetTile(const ElecSim::GridTile* tile,
                               const TileTextureAtlas& textureAtlas) {
  const auto texRect = textureAtlas.GetTileRect(
      tile->GetTileType(),
      tile->GetActivation()
  );
  
  SetTile(tile, texRect);
}

// TODO: Consider creating a TileOperation enum to represent actions like
// SET, ERASE, MOVE, etc. to make the API more consistent and expressive.

void TileChunkManager::SetTiles(
    const std::vector<std::pair<const ElecSim::GridTile*, const sf::IntRect>>&
        tiles) {

  for (const auto& [tile, textureRect] : tiles) {
    SetTile(tile, textureRect);
  }
}

void TileChunkManager::EraseTile(const ElecSim::vi2d& tilePos) {
  // TODO: Refactor duplicated code between SetTile and EraseTile.
  // Consider a common method to find the chunk for a tile position.
  const auto alignToChunkGrid = [](int pos) constexpr noexcept -> int {
    const auto [quot, rem] =
        std::div(pos, static_cast<int>(TileChunk::CHUNK_LENGTH));
    return (quot - (rem < 0)) * static_cast<int>(TileChunk::CHUNK_LENGTH);
  };
  
  const auto chunkBasePos =
      ElecSim::vi2d(alignToChunkGrid(tilePos.x), alignToChunkGrid(tilePos.y));
  
  if (auto it = chunks.find(chunkBasePos); it != chunks.end()) {
    it->second.EraseTile(tilePos);
  }
}

// TODO: Implement a more efficient batch processing for EraseTiles to avoid
// redundant chunk lookups when multiple tiles are in the same chunk.

void TileChunkManager::EraseTiles(const std::vector<ElecSim::vi2d>& tilePositions) {
  for (const auto& tilePos : tilePositions) {
    EraseTile(tilePos);
  }
}

/**
 * @brief Update the chunks with the provided tiles using a texture atlas
 * 
 * This provides a cleaner interface similar to TilePreviewRenderer, reducing
 * the need to manually extract texture rects before adding tiles.
 */
// UpdateTiles method removed - use SetTile with TileTextureAtlas instead

// TODO: Add frustum culling to only draw chunks that are visible in the view.
// This would improve performance for large grids.

void TileChunkManager::draw(sf::RenderTarget& target,
                            sf::RenderStates states) const {
  for (const auto& [chunkPos, chunk] : chunks) {
    target.draw(chunk, states);
  }
}

// TODO: Consider adding a memory management strategy for chunks:
// 1. Unload distant chunks when memory pressure is high
// 2. Only keep actively modified chunks in memory
// 3. Implement chunk serialization for very large grids

}  // namespace Engine