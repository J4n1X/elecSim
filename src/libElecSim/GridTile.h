#pragma once

#include <array>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Common.h"
#include "olcPixelGameEngine.h"

namespace ElecSim {
// Base tile class
// TODO: I want to add a Construct() function that creates a new tile of any
// type, maybe with templates? A factory?
// TODO: Maybe we should not store refNum in here, but this is the easiest way
// right now.
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
  size_t refNum;

 public:
  GridTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f),
           Direction facing = Direction::Top, float size = 1.0f,
           bool defaultActivation = false,
           olc::Pixel inactiveColor = olc::BLACK,
           olc::Pixel activeColor = olc::BLACK);

  virtual ~GridTile() {};

  virtual void Draw(olc::PixelGameEngine* renderer, olc::vf2d screenPos,
                    float screenSize, int alpha = 255);

  // For tiles that need it, this provides interaction logic.
  virtual std::vector<SignalEvent> Init() { return {}; };
  // This has severe side effects. That's fine for actual simulations, but
  // not for preprocessing. 
  virtual std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) = 0;
  // This is called when the user interacts with the tile, e.g. by clicking on it.
  virtual std::vector<SignalEvent> Interact() { return {}; }
  // This has zero side effects, and is used for preprocessing.
  virtual std::vector<SignalEvent> PreprocessSignal(const std::vector<SignalEvent> incomingSignals) = 0;

  void SetPos(olc::vi2d newPos) { pos = newPos; }
  void SetRefNum(size_t newRefNum) { refNum = newRefNum; }
  void SetFacing(Direction newFacing);
  void SetActivation(bool newActivation) { activated = newActivation; }
  void SetDefaultActivation(bool newDefault) { defaultActivation = newDefault; }
  virtual void
  ResetActivation();  // Changed from inline to virtual with implementation

  bool GetActivation() const { return activated; }
  bool GetDefaultActivation() const { return defaultActivation; }
  const olc::vi2d& GetPos() const { return pos; }
  float GetSize() const { return size; }
  const Direction& GetFacing() const { return facing; }
  std::string GetTileInformation() const;
  size_t GetRefNum() const { return refNum; }

  bool CanReceiveFrom(Direction dir) const { return canReceive[dir]; }

  virtual std::string_view TileTypeName() const = 0;
  virtual bool IsEmitter() const = 0;
  virtual bool IsDeterministic() const = 0;
  virtual int GetTileTypeId() const = 0;

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
