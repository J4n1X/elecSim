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

template <typename T>
  requires std::is_arithmetic_v<T>
sf::Vector2<T> ToSfmlVector(const v2d<T>& vec) {
  return sf::Vector2<T>(vec.x, vec.y);
}
// Base class for all tile drawables - polymorphic approach
class TileDrawable : public sf::Drawable, public sf::Transformable {
 public:
  static constexpr float DEFAULT_SIZE = 1.f;

  explicit TileDrawable(
      std::shared_ptr<ElecSim::GridTile> tilePtr,
      std::array<std::shared_ptr<sf::VertexArray>, 2> vArrays);
  ~TileDrawable() = default;

  // Updates the drawable based on the current tile state
  // virtual void UpdateVisualState() = 0;

  [[nodiscard]] std::vector<sf::Vertex> GetVertexArray() const;

  // Gets the tile this drawable represents
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
  // 0 is always inactive, 1 is always active.
  std::shared_ptr<ElecSim::GridTile> tilePtr;
  std::array<std::shared_ptr<sf::VertexArray>, 2> vArrays;
};

class Highlighter : public sf::Drawable, public sf::Transformable {
 public:
  explicit Highlighter(const sf::FloatRect& bounds,
                       sf::Color highlightColor = sf::Color(255, 0, 0, 128))
      : color(highlightColor) {
    rectangle.setPosition({0.f, 0.f});
    rectangle.setSize(bounds.size);
    rectangle.setFillColor(sf::Color::Transparent);
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

// Factory function to create appropriate drawable for any tile type
std::unique_ptr<TileDrawable> CreateTileRenderable(
    std::shared_ptr<ElecSim::GridTile> tilePtr,
    const std::vector<std::shared_ptr<sf::VertexArray>>& vArrays);

sf::Transform GetTileTransform(
    std::shared_ptr<ElecSim::GridTile> const& tilePtr);
sf::Transform GetTileTransform(
    std::unique_ptr<ElecSim::GridTile> const& tilePtr);

}  // namespace Engine