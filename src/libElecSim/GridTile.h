#pragma once

#include <array>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "olcPixelGameEngine.h"

namespace ElecSim {

constexpr int GRIDTILE_BYTESIZE =
    sizeof(int) * 4;  // TileId + Facing + PosX + PosY

enum class Direction { Top = 0, Right = 1, Bottom = 2, Left = 3, Count = 4 };
constexpr std::string_view DirectionToString(Direction dir) {
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
  TileSideStates() { this->fill(false); }
  TileSideStates(std::initializer_list<std::tuple<Direction, bool>> dirs) {
    this->fill(false);
    for (const auto& [dir, val] : dirs) {
      if (dir >= Direction::Top && dir < Direction::Count) {
        (*this)[static_cast<int>(dir)] = val;
      }
    }
  }
  bool& operator[](Direction dir) { return (*this)[static_cast<int>(dir)]; }
  const bool& operator[](Direction dir) const {
    return std::array<bool, static_cast<int>(Direction::Count)>::operator[](static_cast<int>(dir));
  }
};

// Direction utility functions
inline Direction FlipDirection(Direction dir) {
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

struct SignalEvent {
  olc::vi2d sourcePos;
  Direction fromDirection;
  bool isActive;

  SignalEvent(olc::vi2d pos, Direction toDirection, bool active)
      : sourcePos(pos),
        fromDirection(FlipDirection(toDirection)),
        isActive(active){}
  
  // Add a copy constructor to ensure the visitedPositions gets copied
  SignalEvent(const SignalEvent& other)
      : sourcePos(other.sourcePos),
        fromDirection(other.fromDirection),
        isActive(other.isActive) {}
};

// Forward declare GridTile for UpdateEvent
class GridTile;

// Event for the update queue with priority ordering
struct UpdateEvent {
  std::shared_ptr<GridTile> tile;
  SignalEvent event;
  uint32_t updateCycleId;

  UpdateEvent(std::shared_ptr<GridTile> t, const SignalEvent& e, int id)
      : tile(t), event(e), updateCycleId(id) {}

  bool operator<(const UpdateEvent& other) const {
    return updateCycleId < other.updateCycleId;
  }
};

// Base tile class
// TODO: I want to add a Construct() function that creates a new tile of any
// type, maybe with templates? A factory?
class GridTile : public std::enable_shared_from_this<GridTile> {
 protected:
  olc::vi2d pos;
  Direction facing;
  float size;
  bool activated;
  bool defaultActivation;
  olc::Pixel inactiveColor;
  olc::Pixel activeColor;
  TileSideStates canReceive;
  TileSideStates canOutput;
  TileSideStates inputStates;

 public:
  GridTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f),
           Direction facing = Direction::Top, float size = 1.0f,
           bool defaultActivation = false,
           olc::Pixel inactiveColor = olc::BLACK,
           olc::Pixel activeColor = olc::BLACK);

  virtual ~GridTile() {};

  virtual void Draw(olc::PixelGameEngine* renderer, olc::vf2d screenPos,
                    float screenSize, int alpha = 255);

  virtual std::vector<SignalEvent> Init() { return {}; };
  virtual std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) = 0;
  virtual std::vector<SignalEvent> Interact() { return {}; }

  void SetPos(olc::vi2d newPos) { pos = newPos; }
  void SetFacing(Direction newFacing);
  void SetActivation(bool newActivation) { activated = newActivation; }
  void SetDefaultActivation(bool newDefault) { defaultActivation = newDefault; }
  virtual void
  ResetActivation();  // Changed from inline to virtual with implementation

  bool GetActivation() const { return activated; }
  bool GetDefaultActivation() const { return defaultActivation; }
  olc::vi2d GetPos() const { return pos; }
  float GetSize() const { return size; }
  Direction GetFacing() const { return facing; }
  std::string GetTileInformation() const;

  bool CanReceiveFrom(Direction dir) const { return canReceive[dir]; }

  virtual std::string_view TileTypeName() const = 0;
  virtual bool IsEmitter() const = 0;
  virtual int GetTileId() const = 0;

  // Virtual clone method for copying tiles
  virtual std::unique_ptr<GridTile> Clone() const = 0;

  std::array<char, GRIDTILE_BYTESIZE> Serialize();
  static std::unique_ptr<GridTile> Deserialize(
      std::array<char, GRIDTILE_BYTESIZE> data);

  static Direction RotateDirection(Direction dir, Direction facing);
  static std::string_view DirectionToString(Direction dir);
  // Helper function to get the triangle points
  static std::array<olc::vf2d, 3> GetTrianglePoints(olc::vf2d screenPos,
                                                    int screenSize,
                                                    Direction facing);
};

}  // namespace ElecSim
