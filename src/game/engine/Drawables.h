#pragma once

#include <memory>

#include "Common.h"
#include "GridTileTypes.h"
#include "SFML/Graphics.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "SFML/Graphics/Transformable.hpp"
#include "meshes.h"
#include "v2d.h"

namespace Engine {

/**
 * @brief Converts ElecSim::v2d to sf::Vector2.
 * @tparam T Arithmetic type
 * @param vec Vector to convert
 * @return SFML vector
 */
template <typename T>
  requires std::is_arithmetic_v<T>
sf::Vector2<T> ToSfmlVector(const ElecSim::v2d<T>& vec) {
  return sf::Vector2<T>(vec.x, vec.y);
}
/**
 * @class TileDrawable
 * @brief Base class for all tile drawables using a polymorphic approach.
 * 
 * Represents a drawable tile that can be rendered using SFML. Each tile drawable
 * contains a reference to the underlying GridTile and vertex arrays for different states.
 */
class TileDrawable : public sf::Drawable, public sf::Transformable {
 public:
  static constexpr float DEFAULT_SIZE = 1.f;

  explicit TileDrawable(
      std::shared_ptr<ElecSim::GridTile> tilePtr,
      std::array<std::shared_ptr<sf::VertexArray>, 2> vArrays);
  ~TileDrawable() = default;


  [[nodiscard]] std::vector<sf::Vertex> GetVertexArray() const;

  [[nodiscard]] std::shared_ptr<ElecSim::GridTile> GetTile() const noexcept {
    return tilePtr;
  }

  [[nodiscard]] sf::FloatRect getGlobalBounds() const {
    return getTransform().transformRect(
        sf::FloatRect({0.f, 0.f}, {size, size}));
  }

  [[nodiscard]] virtual std::unique_ptr<TileDrawable> Clone() const {
    return std::make_unique<TileDrawable>(*this);
  }

 private:
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
  float size = DEFAULT_SIZE;
  std::shared_ptr<ElecSim::GridTile> tilePtr;
  std::array<std::shared_ptr<sf::VertexArray>, 2> vArrays;  // 0=inactive, 1=active
};

/**
 * @class Highlighter
 * @brief A visual highlight overlay for tiles or areas.
 * 
 * Creates a rectangular highlight with customizable outline and fill colors.
 * Useful for showing selected areas or providing visual feedback.
 */
class Highlighter : public sf::Drawable, public sf::Transformable {
 public:
  explicit Highlighter(const sf::FloatRect& bounds,
                       sf::Color highlightColor = sf::Color(255, 0, 0, 128), sf::Color fillColor = sf::Color(255, 0, 0, 50))
      : color(highlightColor) {
    rectangle.setPosition({0.f, 0.f});
    rectangle.setSize(bounds.size);
    rectangle.setFillColor(fillColor);
    rectangle.setOutlineColor(color);
    rectangle.setOutlineThickness(TileDrawable::DEFAULT_SIZE / 8.f);
  }

  inline void setSize(const sf::Vector2f& newSize) {
    rectangle.setSize(newSize);
  }

  inline void setColor(const sf::Color& newColor) {
    color = newColor;
    rectangle.setOutlineColor(color);
  }

  inline void setFillColor(const sf::Color& fillColor) {
    rectangle.setFillColor(fillColor);
  }

  inline sf::Vector2f getSize() const { return rectangle.getSize(); }

  inline void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
    states.transform *= getTransform();
    target.draw(rectangle, states);
  }

 private:
  sf::RectangleShape rectangle;
  sf::Color color;
};

/**
 * @class TileTextureAtlas
 * @brief Manages a texture atlas for efficient tile rendering.
 * 
 * Creates and manages a texture atlas containing all tile types in both active
 * and inactive states. Designed to render to a render texture for optimal performance.
 */
class TileTextureAtlas {
  public: 
    explicit TileTextureAtlas() = default;
    explicit TileTextureAtlas(uint32_t tilePixelSize);
    [[nodiscard]] inline sf::Texture const& GetTexture() const {
      return renderTarget.getTexture();
    }
    [[nodiscard]]inline sf::IntRect GetTextureRect() const noexcept {
      return sf::IntRect({0, 0}, sf::Vector2i(renderTarget.getSize()));
    }
    [[nodiscard]] sf::IntRect GetTileRect(ElecSim::TileType type, bool activation) const;
    
    /**
     * @brief Get the default texture rectangle for a tile type and activation state.
     * @param type Tile type
     * @param activation Activation state
     * @return IntRect with the default texture coordinates
     */
    static sf::IntRect GetDefaultTileRect(ElecSim::TileType type, bool activation);
    
    inline void SetTilePixelSize(uint32_t newTilePixelSize) {
      tilePixelSize = newTilePixelSize;
      UpdateTextureAtlas();
    }
  private:
    struct TileMesh {
      sf::VertexArray inactiveMesh;
      sf::VertexArray activeMesh;
    };

    void UpdateTextureAtlas();

    sf::RenderTexture renderTarget;
    uint32_t tilePixelSize = 32; // Size of each tile in pixels
    std::vector<TileMesh> meshes; 
};

/**
 * @brief Factory function to create appropriate drawable for any tile type.
 * @param tilePtr Shared pointer to the tile
 * @param vArrays Vector of vertex arrays for different states
 * @return Unique pointer to the created drawable
 */
std::unique_ptr<TileDrawable> CreateTileRenderable(
    std::shared_ptr<ElecSim::GridTile> tilePtr,
    const std::vector<std::shared_ptr<sf::VertexArray>>& vArrays);

/**
 * @brief Creates an SFML transform for positioning and rotating a tile.
 * @param tilePtr Pointer to the tile
 * @return Transform for the tile
 */
sf::Transform GetTileTransform(
    const ElecSim::GridTile * tilePtr);
}  // namespace Engine