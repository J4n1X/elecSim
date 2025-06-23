#pragma once

#include <memory>

#include "Common.h"
#include "GridTileTypes.h"
#include "SFML/Graphics.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Transformable.hpp"
#include "SFML/Graphics/Rect.hpp"
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

  explicit TileDrawable(std::shared_ptr<ElecSim::GridTile> tilePtr)
      : tilePtr(std::move(tilePtr)) {}
  virtual ~TileDrawable() = default;

  // Updates the drawable based on the current tile state
  virtual void UpdateVisualState() = 0;

  // Gets the tile this drawable represents
  [[nodiscard]] std::shared_ptr<ElecSim::GridTile> GetTile() const noexcept {
    return tilePtr;
  }

  [[nodiscard]] sf::FloatRect getGlobalBounds() const {
    return getTransform().transformRect(
        sf::FloatRect({0.f, 0.f}, {size, size}));
  }

  // Pure virtual clone method - must be implemented by all derived classes
  [[nodiscard]] virtual std::unique_ptr<TileDrawable> Clone() const = 0;

 protected:
  float size = DEFAULT_SIZE;
  std::shared_ptr<ElecSim::GridTile> tilePtr;
};

// Basic tile drawable for most tile types - uses square + triangle
class BasicTileDrawable : public TileDrawable {
 public:
  BasicTileDrawable(std::shared_ptr<ElecSim::GridTile> tilePtr,
                    sf::Color activeColor, sf::Color inactiveColor);

  void UpdateVisualState() override;
  [[nodiscard]] std::unique_ptr<TileDrawable> Clone() const override;

 private:
  virtual void draw(sf::RenderTarget& target,
                    sf::RenderStates states) const override;
  sf::VertexArray CreateTileVertexArray();

  sf::VertexArray vArray;
  sf::Color activeColor;
  sf::Color inactiveColor;
};

// Specialized drawable for crossing tiles
class CrossingTileDrawable : public TileDrawable {
 public:
  explicit CrossingTileDrawable(
      std::shared_ptr<ElecSim::CrossingGridTile> initialPtr);
  CrossingTileDrawable(std::shared_ptr<ElecSim::GridTile> initialPtr);

  void UpdateVisualState() final override;
  [[nodiscard]] std::unique_ptr<TileDrawable> Clone() const final override;

 private:
  void Setup();

  sf::RectangleShape baseSquare;
  std::array<sf::RectangleShape, 4> squares;
  const sf::Color activeColor = sf::Color(0, 0, 255);
  const sf::Color inactiveColor = sf::Color(0, 0, 128);
  void draw(sf::RenderTarget& target,
            sf::RenderStates states) const final override;
};


class Highlighter : public sf::Drawable, public sf::Transformable {
 public:
  explicit Highlighter(const sf::FloatRect& bounds, sf::Color color = sf::Color(255, 0, 0, 128)) : color(color) {
    rectangle.setPosition({0.f, 0.f});
    rectangle.setSize(bounds.size);
    rectangle.setFillColor(sf::Color::Transparent);
    rectangle.setOutlineColor(color);
    rectangle.setOutlineThickness(BasicTileDrawable::DEFAULT_SIZE / 8.f);
  }

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
    states.transform *= getTransform();
    target.draw(rectangle, states);
  }

 private:
  static constexpr float size = TileDrawable::DEFAULT_SIZE;
  sf::RectangleShape rectangle;
  sf::Color color;
};

// Factory function to create appropriate drawable for any tile type
std::unique_ptr<TileDrawable> CreateTileDrawable(
    std::shared_ptr<ElecSim::GridTile> tilePtr);

}  // namespace Engine