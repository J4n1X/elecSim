#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <tuple>
#include <format>
#include <iostream>

#include "v2d.h"

/**
 * @brief Concept that checks if a type T is the same as any of the types Ts.
 * @tparam T Type to check
 * @tparam Ts Types to check against
 */
template<typename T, typename... Ts>
concept SameAsAny = (... || std::same_as<T, Ts>);

/**
 * @brief Debug print function that outputs formatted messages in debug builds only.
 * @tparam Args Argument types for formatting
 * @param fmt Format string
 * @param args Arguments to format
 */
template<typename... Args>
inline void DebugPrint([[maybe_unused]]std::format_string<Args...> fmt, [[maybe_unused]]Args&&... args) {
#ifdef DEBUG
    std::cout << std::format(fmt, std::forward<Args>(args)...) << std::endl;
#endif
}

namespace ElecSim {

/**
 * @struct PositionHash
 * @brief Hash functor for vi2d to enable use in unordered containers.
 */
struct PositionHash {
  using is_avalanching = void;
  std::size_t operator()(const vi2d& pos) const;
};

struct TileStateChange {
  vi2d pos;
  bool newState;
  bool operator==(const TileStateChange& other) const {
    return pos == other.pos && newState == other.newState;
  }
};
struct TileStateChangeHash {
  using is_avalanching = void;
  std::size_t operator()(const TileStateChange& change) const;
};

// Forward declaration to avoid circular includes
class GridTile;

constexpr const static size_t GRIDTILE_COUNT = 7;  // Number of tile types
constexpr const int GRIDTILE_BYTESIZE =
    sizeof(int) * 4;  // TileId + Facing + PosX + PosY


enum class Direction { Top = 0, Right = 1, Bottom = 2, Left = 3, Count = 4 };
const char* DirectionToString(Direction dir);

static constexpr std::array<Direction, static_cast<int>(Direction::Count)>
    AllDirections = {Direction::Top, Direction::Right, Direction::Bottom,
                     Direction::Left};

/**
 * @brief Rotates a direction by the specified number of steps.
 * @param dir Direction to rotate
 * @param steps Number of steps (positive = clockwise, negative = counterclockwise)
 * @return Rotated direction
 */
constexpr Direction DirectionRotate(Direction dir, int steps) {
  int count = static_cast<int>(Direction::Count);
  int value = (static_cast<int>(dir) + steps + count) % count;
  // Manual constexpr abs for int
  int abs_value = value < 0 ? -value : value;
  return static_cast<Direction>(abs_value);
}

constexpr Direction DirectionRotate(Direction dir, Direction amount) {
  return DirectionRotate(dir, static_cast<int>(amount));
}

/**
 * @struct TileSideStates
 * @brief Manages the state of all four sides of a tile.
 */
struct TileSideStates
    : private std::array<bool, static_cast<int>(Direction::Count)> {
 private:
  typedef bool T;
  typedef std::array<bool, static_cast<int>(Direction::Count)> array;

 public:
  using array::fill;
  using array::operator[];
  using array::begin;
  using array::end;
  using array::size;
  TileSideStates();
  TileSideStates(std::initializer_list<std::tuple<Direction, bool>> dirs);
  bool& operator[](Direction dir);
  const bool& operator[](Direction dir) const;
};

/**
 * @brief Flips a direction to its opposite (e.g., Top becomes Bottom).
 * @param dir Direction to flip
 * @return Opposite direction
 */
Direction FlipDirection(Direction dir);

struct SignalEvent {
  vi2d sourcePos;
  Direction fromDirection;
  bool isActive;

  SignalEvent(vi2d pos, Direction toDirection, bool active);

  // Add a copy constructor to ensure the visitedPositions gets copied
  SignalEvent(const SignalEvent& other);
  SignalEvent& operator=(const SignalEvent& other);
};

/**
 * @struct UpdateEvent
 * @brief Event for the update queue with priority ordering.
 */
struct UpdateEvent {
  std::shared_ptr<GridTile> tile;
  SignalEvent event;
  uint32_t updateCycleId;

  UpdateEvent(std::shared_ptr<GridTile> t, const SignalEvent& e, int id);

  bool operator<(const UpdateEvent& other) const;
};

/**
 * @brief Translates a position by one unit in the specified direction.
 * @param pos Starting position
 * @param dir Direction to move
 * @return New position
 */
vi2d TranslatePosition(vi2d pos, Direction dir);

/**
 * @brief Calculates the direction from one position to another.
 * @param from Starting position
 * @param to Target position
 * @return Direction from 'from' to 'to'
 */
Direction DirectionFromVectors(vi2d from, vi2d to);

}  // namespace ElecSim