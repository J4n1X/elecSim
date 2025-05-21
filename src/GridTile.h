#pragma once

#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "olcPixelGameEngine.h"

constexpr int GRIDTILE_BYTESIZE =
    sizeof(int) * 4;  // TileId + Facing + PosX + PosY

enum class Direction { Top = 0, Right = 1, Bottom = 2, Left = 3, Count = 4 };

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

  SignalEvent(olc::vi2d pos, Direction dir, bool active)
      : sourcePos(pos), fromDirection(FlipDirection(dir)), isActive(active) {}
};

// Forward declare GridTile for UpdateEvent
class GridTile;

// Event for the update queue with priority ordering
struct UpdateEvent {
  std::shared_ptr<GridTile> tile;
  SignalEvent event;
  int priority;

  UpdateEvent(std::shared_ptr<GridTile> t, const SignalEvent& e, int p = 0)
      : tile(t), event(e), priority(p) {}

  bool operator<(const UpdateEvent& other) const {
    return priority < other.priority;
  }
};

// Base tile class
class GridTile : public std::enable_shared_from_this<GridTile> {
 protected:
  olc::vi2d pos;
  Direction facing;
  float size;
  bool activated;
  bool defaultActivation;
  olc::Pixel inactiveColor;
  olc::Pixel activeColor;
  bool canReceive[static_cast<int>(Direction::Count)];
  bool canOutput[static_cast<int>(Direction::Count)];

 public:
  GridTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f),
           Direction facing = Direction::Top, float size = 1.0f,
           bool defaultActivation = false,
           olc::Pixel inactiveColor = olc::BLACK,
           olc::Pixel activeColor = olc::BLACK);

  virtual ~GridTile() = default;

  void Draw(olc::PixelGameEngine* renderer, olc::vi2d screenPos,
            int screenSize);
  virtual std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) = 0;
  virtual std::vector<SignalEvent> Interact() { return {}; }

  void SetPos(olc::vi2d newPos) { pos = newPos; }
  void SetFacing(Direction newFacing);
  void SetActivation(bool newActivation) { activated = newActivation; }
  void SetDefaultActivation(bool newDefault) { defaultActivation = newDefault; }
  virtual void ResetActivation() { activated = defaultActivation; }

  bool GetActivation() const { return activated; }
  bool GetDefaultActivation() const { return defaultActivation; }
  olc::vi2d GetPos() const { return pos; }
  float GetSize() const { return size; }
  Direction GetFacing() const { return facing; }
  std::string GetTileInformation() const;

  bool CanReceiveFrom(Direction dir) const {
    return canReceive[static_cast<int>(dir)];
  }
  bool CanSendTo(Direction dir) const {
    return canOutput[static_cast<int>(dir)];
  }

  virtual std::string_view TileTypeName() const = 0;
  virtual bool IsEmitter() const = 0;
  virtual int GetTileId() const = 0;

  std::array<char, GRIDTILE_BYTESIZE> Serialize();
  static std::shared_ptr<GridTile> Deserialize(
      std::array<char, GRIDTILE_BYTESIZE> data);

  static Direction RotateDirection(Direction dir, Direction facing);
  static std::string_view DirectionToString(Direction dir);
  // Helper function to get the triangle points
  static std::array<olc::vf2d, 3> GetTrianglePoints(olc::vf2d screenPos,
                                                    int screenSize,
                                                    Direction facing);
};
