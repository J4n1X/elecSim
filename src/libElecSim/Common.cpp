#include "Common.h"

namespace ElecSim {

std::string_view DirectionToString(Direction dir) {
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

SignalEvent::SignalEvent(olc::vi2d pos, Direction toDirection, bool active)
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

}  // namespace ElecSim
