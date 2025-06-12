#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <tuple>

#include "olcPixelGameEngine.h"

// Formatter for olc::v_2d
template <typename T>
struct std::formatter<olc::v_2d<T>, char> {
  int decimals = 0;  // Number of decimal places to format

  constexpr std::format_parse_context::iterator parse(
      std::format_parse_context& ctx) {
    auto it = ctx.begin();
    auto end = ctx.end();
    // Check for an opening brace for format specifiers
    if (it != end && *it == ':') {
      ++it;  // Consume the ':'
      // If we have a number here, that's the number of decimals
      while (it != end && std::isdigit(*it)) {
        decimals = decimals * 10 + (*it - '0');
        ++it;
      }
      // Loop until a '}' or end of string, consuming characters
      while (it != end && *it != '}') {
        ++it;
      }
    }

    // Return the iterator to the end of the format specifier.
    return it;
  }
  std::format_context::iterator format(const olc::v_2d<T>& vec,
                                       std::format_context& ctx) const {
    if constexpr (std::is_floating_point_v<T>) {
      if (decimals >= 0) {
        return std::format_to(ctx.out(), "({:.{}f}, {:.{}f})", vec.x, decimals,
                              vec.y, decimals);
      }
    } else {
      return std::format_to(ctx.out(), "({}, {})", vec.x, vec.y);
    }
  }
};

namespace ElecSim {

// Hash functor for olc::vi2d to use in unordered sets/maps
struct PositionHash {
  using is_avalanching = void;
  std::size_t operator()(const olc::vi2d& pos) const;
};

// Equals functor for olc::vi2d
struct PositionEqual {
  bool operator()(const olc::vi2d& lhs, const olc::vi2d& rhs) const;
};

// Forward declaration to avoid circular includes
class GridTile;

constexpr int GRIDTILE_BYTESIZE =
    sizeof(int) * 4;  // TileId + Facing + PosX + PosY

enum class Direction { Top = 0, Right = 1, Bottom = 2, Left = 3, Count = 4 };
std::string_view DirectionToString(Direction dir);

static constexpr std::array<Direction, static_cast<int>(Direction::Count)>
    AllDirections = {Direction::Top, Direction::Right, Direction::Bottom,
                     Direction::Left};

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
  olc::vi2d sourcePos;
  Direction fromDirection;
  bool isActive;

  SignalEvent(olc::vi2d pos, Direction toDirection, bool active);

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

olc::vi2d TranslatePosition(olc::vi2d pos, Direction dir);
Direction DirectionFromVectors(olc::vi2d from, olc::vi2d to);
}  // namespace ElecSim