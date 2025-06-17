#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <tuple>

#include "v2d.h"

// Check if constraint is satisfied for all these types.
template<typename T, typename... Ts>
concept SameAsAny = (... || std::same_as<T, Ts>);

namespace ElecSim {

// Hash functor for vi2d to use in unordered sets/maps
struct PositionHash {
  using is_avalanching = void;
  std::size_t operator()(const vi2d& pos) const;
};

// Equals functor for vi2d
struct PositionEqual {
  bool operator()(const vi2d& lhs, const vi2d& rhs) const;
};

// Forward declaration to avoid circular includes
class GridTile;

constexpr int GRIDTILE_BYTESIZE =
    sizeof(int) * 4;  // TileId + Facing + PosX + PosY

enum class Direction { Top = 0, Right = 1, Bottom = 2, Left = 3, Count = 4 };
const char* DirectionToString(Direction dir);

static constexpr std::array<Direction, static_cast<int>(Direction::Count)>
    AllDirections = {Direction::Top, Direction::Right, Direction::Bottom,
                     Direction::Left};

// Rotate (steps) times. Can be positive or negative.
// Positive = rotate right.
// Negative = rotate left.
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

// This is used only to inform if an update happened or not
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

// Direction utility functions
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

// Event for the update queue with priority ordering.
struct UpdateEvent {
  std::shared_ptr<GridTile> tile;
  SignalEvent event;
  uint32_t updateCycleId;

  UpdateEvent(std::shared_ptr<GridTile> t, const SignalEvent& e, int id);

  bool operator<(const UpdateEvent& other) const;
};

vi2d TranslatePosition(vi2d pos, Direction dir);
Direction DirectionFromVectors(vi2d from, vi2d to);
}  // namespace ElecSim