#include "TileChunkManager.h"

#include "Common.h"
#include "Drawables.h"

namespace Engine {
constexpr static int AlignToChunkGrid(int pos) noexcept {
  const auto [quot, rem] =
      std::div(pos, static_cast<int>(TileChunk::CHUNK_LENGTH));
  return (quot - (rem < 0)) * static_cast<int>(TileChunk::CHUNK_LENGTH);
}

// TODO: Unify the chunk management API across the codebase. Current API has
// several inconsistencies in parameter types and lacks a cohesive design
// pattern. Consider creating a unified interface that handles both grid and
// visual representation.

void TileChunkManager::SetTile(const ElecSim::GridTile* tile,
                               sf::IntRect textureRect) {
  const auto& tilePos = tile->GetPos();
  const auto chunkBasePos =
      ElecSim::vi2d(AlignToChunkGrid(tilePos.x), AlignToChunkGrid(tilePos.y));

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
  const auto texRect =
      textureAtlas.GetTileRect(tile->GetTileType(), tile->GetActivation());
  const auto& tilePos = tile->GetPos();
  const auto chunkBasePos =
      ElecSim::vi2d(AlignToChunkGrid(tilePos.x), AlignToChunkGrid(tilePos.y));

  if (auto it = chunks.find(chunkBasePos); it != chunks.end()) {
    it->second.SetTexture(&textureAtlas.GetTexture());
    it->second.SetTile(tile, texRect);
  } else {  // Construct new chunk if it doesn't exist yet.
    TileChunk newChunk(sf::Vector2f(chunkBasePos.x, chunkBasePos.y), &textureAtlas.GetTexture());
    newChunk.SetTile(tile, texRect);
    chunks.emplace(chunkBasePos, std::move(newChunk));
  }
}

void TileChunkManager::SetTiles(
    const std::vector<std::pair<const ElecSim::GridTile*, const sf::IntRect>>&
        tiles) {
  for (const auto& [tile, textureRect] : tiles) {
    SetTile(tile, textureRect);
  }
}

void TileChunkManager::EraseTile(const ElecSim::vi2d& tilePos) {
  const auto chunkBasePos =
      ElecSim::vi2d(AlignToChunkGrid(tilePos.x), AlignToChunkGrid(tilePos.y));

  if (auto it = chunks.find(chunkBasePos); it != chunks.end()) {
    it->second.EraseTile(tilePos);
  }
}

// This approach is a little slow, but it also is simple.
void TileChunkManager::EraseTiles(
    const std::vector<ElecSim::vi2d>& tilePositions) {
  for (const auto& tilePos : tilePositions) {
    EraseTile(tilePos);
  }
}

bool TileChunkManager::IsChunkVisible(const sf::Vector2f& chunkWorldPos, const sf::View& view) const {
  // Get view bounds in world coordinates
  const sf::Vector2f viewCenter = view.getCenter();
  const sf::Vector2f viewSize = view.getSize();
  const sf::Vector2f viewTopLeft = viewCenter - viewSize * 0.5f;
  const sf::Vector2f viewBottomRight = viewCenter + viewSize * 0.5f;
  
  // Calculate chunk bounds
  const sf::Vector2f chunkSize(TileChunk::CHUNK_LENGTH * TileChunk::TILE_WORLD_SIZE,
                               TileChunk::CHUNK_LENGTH * TileChunk::TILE_WORLD_SIZE);
  const sf::Vector2f chunkBottomRight = chunkWorldPos + chunkSize;
  
  // Check for intersection (AABB collision detection)
  return !(chunkWorldPos.x >= viewBottomRight.x || 
           chunkBottomRight.x <= viewTopLeft.x ||
           chunkWorldPos.y >= viewBottomRight.y || 
           chunkBottomRight.y <= viewTopLeft.y);
}

void TileChunkManager::RenderVisibleChunks(sf::RenderTarget& target, sf::RenderStates states,
                                          const sf::View& view, [[maybe_unused]] const sf::Texture* texture) const {
  // Don't override states.texture here since TileChunk manages its own texture
  // The texture parameter is kept for future use or if chunks don't have texture set
  
  for (const auto& [chunkPos, chunk] : chunks) {
    const sf::Vector2f chunkWorldPos(static_cast<float>(chunkPos.x), static_cast<float>(chunkPos.y));
    
    if (IsChunkVisible(chunkWorldPos, view)) {
      target.draw(chunk, states);
    }
  }
}

// TODO: Consider adding a memory management strategy for chunks:
// 1. Unload distant chunks when memory pressure is high
// 2. Only keep actively modified chunks in memory
// 3. Implement chunk serialization for very large grids
// 4. Add spatial indexing (e.g., quadtree) for faster visibility queries with many chunks

}  // namespace Engine