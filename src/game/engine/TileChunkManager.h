#pragma once
#include "TileChunk.h"
#include "ankerl/unordered_dense.h"
#include "Drawables.h"

namespace Engine {
class TileChunkManager : public sf::Drawable {
 public:
  TileChunkManager() = default;
  // No longer need to store the texture atlas pointer
  ~TileChunkManager() = default;
  // Sets a tile in the correct chunk with explicit texture rect.
  void SetTile(const ElecSim::GridTile* tile, sf::IntRect textureRect);
  
  /**
   * @brief Sets a tile in the correct chunk using the texture atlas to get the texture rect
   * @param tile The tile to add
   * @param textureAtlas Reference to the texture atlas for getting the texture rect
   */
  void SetTile(const ElecSim::GridTile* tile, const TileTextureAtlas& textureAtlas);
  // Sets many tiles in the correct chunks.
  void SetTiles(
      const std::vector<std::pair<const ElecSim::GridTile*, const sf::IntRect>>&
          tiles);
          
  // UpdateTiles method removed - use SetTile with TileTextureAtlas instead
  // Erases a tile from the correct chunk.
  void EraseTile(const ElecSim::vi2d& tilePos);
  // Erases multiple tiles from the chunks.
  void EraseTiles(const std::vector<ElecSim::vi2d>& tilePositions);
  void clear() noexcept {
    chunks.clear();
  }
 private:
  using ChunkMap = ankerl::unordered_dense::map<ElecSim::vi2d, TileChunk,
                                                ElecSim::PositionHash>;
  // The map is going to be indexed by grid coordinates. The chunks hold where
  // they are in the world.
  ChunkMap chunks;
  // We no longer need to store the texture pointer since we can get it from TileTextureAtlas

  void draw(sf::RenderTarget& target, 
            sf::RenderStates states) const final override;
};

}  // namespace Engine