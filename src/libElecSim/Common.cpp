#include "Common.h"
#include "ankerl/unordered_dense.h"

namespace ElecSim {
std::size_t PositionHash::operator()(const vi2d& pos) const {
  using ankerl::unordered_dense::detail::wyhash::hash;
  return hash(&pos, sizeof(vi2d));
}

std::size_t TileStateChangeHash::operator()(
    const TileStateChange& change) const {
  using ankerl::unordered_dense::detail::wyhash::hash;
  return hash(&change, sizeof(TileStateChange));
}


const char* DirectionToString(Direction dir) {
  switch (dir) {
    case Direction::Top:
      return "Top";
    case Direction::Right:
      return "Right";
    case Direction::Bottom:
      return "Bottom";
    case Direction::Left:
      return "Left";
    default:
      return "Unknown";
  }
}

TileSideStates::TileSideStates() {
  this->fill(false);
}

TileSideStates::TileSideStates(std::initializer_list<std::tuple<Direction, bool>> dirs) {
  this->fill(false);
  for (const auto& [dir, val] : dirs) {
    if (dir >= Direction::Top && dir < Direction::Count) {
      (*this)[static_cast<int>(dir)] = val;
    }
  }
}

bool& TileSideStates::operator[](Direction dir) {
  return (*this)[static_cast<int>(dir)];
}

const bool& TileSideStates::operator[](Direction dir) const {
  return std::array<bool, static_cast<int>(Direction::Count)>::operator[](
      static_cast<int>(dir));
}

Direction FlipDirection(Direction dir) {
  switch (dir) {
    case Direction::Top:
      return Direction::Bottom;
    case Direction::Bottom:
      return Direction::Top;
    case Direction::Left:
      return Direction::Right;
    case Direction::Right:
      return Direction::Left;
    default:
      return dir;
  }
}

SignalEvent::SignalEvent(vi2d pos, Direction toDirection, bool active)
    : sourcePos(pos),
      fromDirection(FlipDirection(toDirection)),
      isActive(active) {}

SignalEvent::SignalEvent(const SignalEvent& other)
    : sourcePos(other.sourcePos),
      fromDirection(other.fromDirection),
      isActive(other.isActive) {}

SignalEvent& SignalEvent::operator=(const SignalEvent& other) {
  if (this != &other) {
    sourcePos = other.sourcePos;
    fromDirection = other.fromDirection;
    isActive = other.isActive;
  }
  return *this;
}

// Implementation of UpdateEvent methods
UpdateEvent::UpdateEvent(std::shared_ptr<GridTile> t, const SignalEvent& e, int id)
    : tile(t), event(e), updateCycleId(id) {}

bool UpdateEvent::operator<(const UpdateEvent& other) const {
  return updateCycleId < other.updateCycleId;
}

vi2d TranslatePosition(vi2d pos, const Direction dir) {
  switch (dir) {
    case Direction::Top:
      return pos + vi2d(0, -1);
    case Direction::Right:
      return pos + vi2d(1, 0);
    case Direction::Bottom:
      return pos + vi2d(0, 1);
    case Direction::Left:
      return pos + vi2d(-1, 0);
    default:
      return pos;
  }
}

Direction DirectionFromVectors(vi2d from, vi2d to) {
  vi2d diff = to - from;
  if (diff.x == 0 && diff.y < 0) return Direction::Top;
  if (diff.x > 0 && diff.y == 0) return Direction::Right;
  if (diff.x == 0 && diff.y > 0) return Direction::Bottom;
  if (diff.x < 0 && diff.y == 0) return Direction::Left;
  throw std::invalid_argument(
      std::format("Invalid direction: from and to vectors do not form a valid direction. "
                  "from: ({}, {}), to: ({}, {}) --> ({}, {})",
                  from.x, from.y, to.x, to.y, diff.x, diff.y));
  }
}  // namespace ElecSim
