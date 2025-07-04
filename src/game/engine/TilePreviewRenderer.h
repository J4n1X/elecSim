#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Drawables.h"
#include "GridTile.h"
#include "SFML/Graphics.hpp"
#include "TileChunkManager.h"
#include "v2d.h"

namespace Engine {
/**
 * @class TilePreviewRenderer
 * @brief Handles rendering of tile previews with transparency effects.
 * 
 * This class manages a separate chunk manager for preview rendering and
 * handles shader-based transparency for preview tiles.
 */
class TilePreviewRenderer : public sf::Drawable, public sf::Transformable {
 public:
  TilePreviewRenderer();
  ~TilePreviewRenderer();

  /**
   * @brief Initialize the preview renderer
   */
  void Initialize();

  /**
   * @brief Update the preview rendering with the provided tiles
   * @param tiles Reference to the original tiles to preview
   * @param textureAtlas Reference to the texture atlas for getting tile rects
   * @note Should only be called when tiles have been modified (copied, rotated, created)
   * @note The rendering position is controlled by sf::Transformable::setPosition
   */
  void UpdatePreview(
      const std::vector<std::unique_ptr<ElecSim::GridTile>>& tiles,
      const TileTextureAtlas& textureAtlas);

  /**
   * @brief Clear the current preview
   */
  void ClearPreview();

  /**
   * @brief Draw the preview to the render target (sf::Drawable implementation)
   * @param target Render target to draw to
   * @param states Render states to use (will apply the transform from
   * sf::Transformable)
   */
  virtual void draw(sf::RenderTarget& target,
                    sf::RenderStates states) const override;

  /**
   * @brief Check if shaders are available and working
   * @return true if transparency effects are available
   */
  bool IsTransparencyAvailable() const;

  /**
   * @brief Set the transparency level for previews
   * @param alpha Alpha value (0-255, where 0 is fully transparent, 255 is
   * opaque)
   */
  void SetPreviewAlpha(uint8_t alpha);

  /**
   * @brief Debug function to print the current state of the renderer
   */
  void DebugPrintState() const;

 private:
  TileChunkManager previewChunkManager;
  std::unique_ptr<sf::Shader> alphaShader;
  uint8_t previewAlpha;
  bool shadersAvailable;

  /**
   * @brief Initialize the alpha shader for transparency effects
   * @return true if shader was successfully created
   */
  bool InitializeShader();
};

}  // namespace Engine
