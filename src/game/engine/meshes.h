#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

#include "Common.h"
#include "GridTile.h"
#include "SFML/Graphics.hpp"

namespace Engine {

/**
 * @class TileTypeIndexable
 * @brief Container wrapper that allows indexing by TileType.
 * @tparam Container The underlying container type
 */
template <typename Container>
class TileTypeIndexable {
 public:
  using value_type = typename Container::value_type;
  using container_type = Container;

  // Default constructor
  constexpr TileTypeIndexable() = default;

  // Copy/move constructors for existing containers
  constexpr TileTypeIndexable(const Container& container) : data(container) {}
  constexpr TileTypeIndexable(Container&& container)
      : data(std::move(container)) {}

  // Initializer list constructor for implicit construction
  constexpr TileTypeIndexable(std::initializer_list<value_type> list) {
    if (list.size() != ElecSim::GRIDTILE_COUNT) {
      throw std::runtime_error(
          "TileTypeIndexable must have exactly 7 elements.");
    }
    auto it = list.begin();
    for (size_t i = 0; i < ElecSim::GRIDTILE_COUNT; ++i, ++it) {
      data[i] = *it;
    }
  }

  // Perfect forwarding constructor for any container constructor arguments
  template <typename... Args>
  constexpr TileTypeIndexable(Args&&... args)
      : data{std::forward<Args>(args)...} {
    if (data.size() != ElecSim::GRIDTILE_COUNT) {
      throw std::runtime_error(
          "TileTypeIndexable must have exactly 7 elements.");
    }
  }

  // TileType indexing
  [[nodiscard]] constexpr value_type& operator[](
      ElecSim::TileType type) noexcept {
    return data[static_cast<size_t>(type)];
  }
  [[nodiscard]] constexpr const value_type& operator[](
      ElecSim::TileType type) const noexcept {
    return data[static_cast<size_t>(type)];
  }

  // Regular indexing
  [[nodiscard]] constexpr value_type& operator[](size_t index) noexcept {
    return data[index];
  }
  [[nodiscard]] constexpr const value_type& operator[](
      size_t index) const noexcept {
    return data[index];
  }

  // Container interface
  [[nodiscard]] constexpr size_t size() const noexcept { return data.size(); }
  [[nodiscard]] constexpr auto begin() noexcept { return data.begin(); }
  [[nodiscard]] constexpr auto end() noexcept { return data.end(); }
  [[nodiscard]] constexpr auto begin() const noexcept { return data.begin(); }
  [[nodiscard]] constexpr auto end() const noexcept { return data.end(); }

  // Access to underlying container
  [[nodiscard]] constexpr Container& get() noexcept { return data; }
  [[nodiscard]] constexpr const Container& get() const noexcept { return data; }

 private:
  Container data;
};

/**
 * @class TileColors
 * @brief Manages active and inactive colors for tiles.
 */
class TileColors {
 public:
  constexpr TileColors()
      : data{sf::Color::Transparent, sf::Color::Transparent} {}
  constexpr TileColors(sf::Color active, sf::Color inactive)
      : data{active, inactive} {}

  constexpr TileColors(std::initializer_list<sf::Color> colors) {
    if (colors.size() != 2) {
      throw std::runtime_error("TileColors must have exactly 2 colors.");
    }
    auto it = colors.begin();
    data[0] = *it;      // Active color
    data[1] = *(++it);  // Inactive color
  }
  [[nodiscard]] constexpr sf::Color& GetActive() noexcept { return data[0]; }
  [[nodiscard]] constexpr sf::Color& GetInactive() noexcept { return data[1]; }
  [[nodiscard]] constexpr const sf::Color& operator[](
      bool isActive) const noexcept {
    return data[isActive ? 0 : 1];
  }
  [[nodiscard]] constexpr sf::Color& operator[](bool isActive) noexcept {
    return data[isActive ? 0 : 1];
  }

 private:
  std::array<sf::Color, 2> data;
};

/**
 * @class TileModel
 * @brief Represents the visual model of a tile with geometry and colors.
 */
class TileModel {
 private:
  TileColors colors;
  const std::array<sf::Vector2f, 3>* triangles_data;
  std::size_t triangles_count;

 public:
  constexpr TileModel()
      : colors{}, triangles_data(nullptr), triangles_count(0) {}

  template <std::size_t N>
  constexpr TileModel(
      const std::array<std::array<sf::Vector2f, 3>, N>& triangles,
      const TileColors newColors)
      : colors(newColors), triangles_data(triangles.data()), triangles_count(N) {
    if (N < 1) {
      throw std::runtime_error("TileModel must have at least one triangle.");
    }
  }

  [[nodiscard]] constexpr const TileColors& GetColors() const noexcept {
    return colors;
  }

  [[nodiscard]] std::vector<sf::Vertex> GetVertices(sf::Transform transform,
                                                    bool activated) const;
};

// clang-format off
constexpr const static TileTypeIndexable<std::array<TileColors, 7>>
    TILE_COLORS = {
      {sf::Color(128, 128, 000, 255), sf::Color(255, 255, 255, 255)},
      {sf::Color(255, 255, 000, 255), sf::Color(192, 192, 192, 255)},
      {sf::Color(000, 255, 255, 255), sf::Color(000, 128, 128, 255)},
      {sf::Color(000, 255, 000, 255), sf::Color(000, 128, 000, 255)},
      {sf::Color(255, 000, 000, 255), sf::Color(128, 000, 000, 255)},
      {sf::Color(255, 000, 255, 255), sf::Color(128, 000, 128, 255)},
      {sf::Color(000, 000, 128, 255), sf::Color(000, 000, 000, 000)}
    };
// clang-format on

// Size of each tile in world units
constexpr const static float TILE_SIZE = 1.f;

// clang-format off
constexpr const static std::array<std::array<sf::Vector2f, 3>, 3>
    _TILE_WITH_ARROW_MODEL = {{
      // Triangle one (inactive by default)
      {sf::Vector2f(0.f, 0.f), sf::Vector2f(TILE_SIZE / 2.f, 0.f),sf::Vector2f(0.f, TILE_SIZE)},
      // Triangle two (active by default)
      {sf::Vector2f(TILE_SIZE / 2.f, 0.f),sf::Vector2f(TILE_SIZE, TILE_SIZE), sf::Vector2f(0.f, TILE_SIZE)},
      // Triangle three (inactive by default)
      {sf::Vector2f(TILE_SIZE / 2.f, 0.f), sf::Vector2f(TILE_SIZE, 0.f),sf::Vector2f(TILE_SIZE, TILE_SIZE)}
    }};
// clang-format on

#define trenchSize (1.f / 8.f)
#define squareSize ((TILE_SIZE - trenchSize) / 2.f)
#define offset (squareSize + trenchSize)
#define QUAD(pos)                                   \
  {pos, pos + sf::Vector2f(squareSize, 0.f),        \
   pos + sf::Vector2f(0.f, squareSize)},            \
  {                                                 \
    pos + sf::Vector2f(squareSize, 0.f),            \
        pos + sf::Vector2f(squareSize, squareSize), \
        pos + sf::Vector2f(0.f, squareSize)         \
  }
constexpr const static std::array<std::array<sf::Vector2f, 3>, 8>
    _CROSSING_MODEL = {{
        QUAD(sf::Vector2f(0.f, 0.f)),
        QUAD(sf::Vector2f(offset, 0.f)),
        QUAD(sf::Vector2f(0.f, offset)),
        QUAD(sf::Vector2f(offset, offset)),
    }};
#undef QUAD
#undef offset
#undef trenchSize
#undef squareSize

// clang-format off
constexpr static TileTypeIndexable<
    std::array<TileModel, ElecSim::GRIDTILE_COUNT>>
    TILE_MODELS = {
        TileModel(_TILE_WITH_ARROW_MODEL, TILE_COLORS[ElecSim::TileType::Wire]),
        TileModel(_TILE_WITH_ARROW_MODEL, TILE_COLORS[ElecSim::TileType::Junction]),
        TileModel(_TILE_WITH_ARROW_MODEL, TILE_COLORS[ElecSim::TileType::Emitter]),
        TileModel(_TILE_WITH_ARROW_MODEL, TILE_COLORS[ElecSim::TileType::SemiConductor]),
        TileModel(_TILE_WITH_ARROW_MODEL, TILE_COLORS[ElecSim::TileType::Button]),
        TileModel(_TILE_WITH_ARROW_MODEL, TILE_COLORS[ElecSim::TileType::Inverter]),
        TileModel(_CROSSING_MODEL,        TILE_COLORS[ElecSim::TileType::Crossing])};
// clang-format on

/**
 * @class MeshLoader
 * @brief Loads mesh data from files or strings and creates SFML vertex arrays.
 * 
 * Expected format:
 * - First line: "type <PrimitiveType>"
 * - Following lines: "v <x> <y> c <r> <g> <b> <a> | <r> <g> <b> <a> | ..."
 * 
 * Multiple color variants per vertex are supported (separated by '|').
 */
class MeshLoader {
 public:
  void LoadMeshFromFile(const std::filesystem::path& filePath);
  void LoadMeshFromString(const std::string& fileContent);
  [[nodiscard]] std::vector<std::shared_ptr<sf::VertexArray>> const& GetMeshes()
      const {
    return meshes;
  }
  [[nodiscard]] std::shared_ptr<sf::VertexArray> const& GetMesh(
      size_t index) const;
  [[nodiscard]] std::shared_ptr<sf::VertexArray> const& operator[](
      size_t index) const {
    return GetMesh(index);
  }
  [[nodiscard]] size_t GetMeshCount() const noexcept { return meshes.size(); }

 private:
  std::vector<std::shared_ptr<sf::VertexArray>> meshes;
  [[nodiscard]] std::vector<sf::VertexArray> ParseMesh(
      std::istream& stream) const;
};
}  // namespace Engine
