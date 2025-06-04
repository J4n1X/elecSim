#pragma once

#include <memory>
#include <array>
#include <tuple>
#include <initializer_list>
#include "olcPixelGameEngine.h"

namespace ElecSim {
using vi2d = olc::vi2d;

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

// Event for the update queue with priority ordering
struct UpdateEvent {
  std::shared_ptr<GridTile> tile;
  SignalEvent event;
  uint32_t updateCycleId;

  UpdateEvent(std::shared_ptr<GridTile> t, const SignalEvent& e, int id);
  
  bool operator<(const UpdateEvent& other) const;
};
}  // namespace ElecSim