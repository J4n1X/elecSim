#pragma once
#include "TileChunk.h"
#include "ankerl/unordered_dense.h"
#include "Drawables.h"

namespace Engine {
/**
 * @class TileChunkManager
 * @brief Manages a collection of TileChunks for efficient rendering of tiles in a grid-based system.
 * 
 * The TileChunkManager is responsible for organizing tiles into chunks based on their positions,
 * which allows for efficient rendering by only drawing visible chunks. It provides frustum culling
 * to render only chunks visible in the current view.
 * 
 * This class provides methods to add, update, and remove tiles from the appropriate chunks,
 * abstracting away the chunk management logic from the client code. Tiles can be added either
 * with explicit texture rectangles or by using a TileTextureAtlas to look up the appropriate
 * texture regions.
 * 
 * The manager uses an efficient hash map implementation (ankerl::unordered_dense::map) to store
 * chunks indexed by their grid coordinates.
 */
class TileChunkManager {
 public:
  TileChunkManager() = default;
  ~TileChunkManager() = default;
  
  /**
   * @brief Sets a tile in the correct chunk with explicit texture rect.
   * @param tile The tile to add
   * @param textureRect Texture rectangle for the tile
   */
  void SetTile(const ElecSim::GridTile* tile, sf::IntRect textureRect);
  
  /**
   * @brief Sets a tile in the correct chunk using the texture atlas to get the texture rect
   * @param tile The tile to add
   * @param textureAtlas Reference to the texture atlas for getting the texture rect
   */
  void SetTile(const ElecSim::GridTile* tile, const TileTextureAtlas& textureAtlas);
  /**
   * @brief Sets many tiles in the correct chunks.
   * @param tiles Vector of tile-texture rectangle pairs
   */
  void SetTiles(
      const std::vector<std::pair<const ElecSim::GridTile*, const sf::IntRect>>&
          tiles);
          
  void EraseTile(const ElecSim::vi2d& tilePos);
  void EraseTiles(const std::vector<ElecSim::vi2d>& tilePositions);
  void clear() noexcept {
    chunks.clear();
  }

  /**
   * @brief Render all chunks visible within the given view frustum.
   * @param target Render target to draw to
   * @param states Render states
   * @param view The current view for frustum culling
   * @param texture Texture atlas to use for rendering
   */
  void RenderVisibleChunks(sf::RenderTarget& target, sf::RenderStates states, 
                          const sf::View& view, const sf::Texture* texture) const;

  /**
   * @brief Get all chunks for manual rendering (without culling).
   * @return Const reference to the chunks map
   */
  const auto& GetChunks() const noexcept { return chunks; }

 private:
  using ChunkMap = ankerl::unordered_dense::map<ElecSim::vi2d, TileChunk,
                                                ElecSim::PositionHash>;
  ChunkMap chunks;

  /**
   * @brief Check if a chunk at the given world position is visible in the view.
   * @param chunkWorldPos World position of the chunk
   * @param view The current view
   * @return True if the chunk intersects with the view bounds
   */
  bool IsChunkVisible(const sf::Vector2f& chunkWorldPos, const sf::View& view) const;
};

}  // namespace Engine