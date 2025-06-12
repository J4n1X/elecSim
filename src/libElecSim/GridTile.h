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
  // This is called when the user interacts with the tile, e.g. by clicking on
  // it.
  virtual std::vector<SignalEvent> Interact() { return {}; }
  // This has zero side effects, and is used for preprocessing.
  virtual std::vector<SignalEvent> PreprocessSignal(
      const SignalEvent incomingSignal) = 0;

  void SetPos(olc::vi2d newPos) { pos = newPos; }
  void SetActivation(bool newActivated) { activated = newActivated; }
  void SetFacing(Direction newFacing);
  void SetDefaultActivation(bool newDefault) { defaultActivation = newDefault; }
  void ToggleInputState(Direction dir) {
    if (canReceive[dir]) {
      inputStates[dir] = !inputStates[dir];
    }
  }
  virtual void
  ResetActivation();  // Changed from inline to virtual with implementation

  bool GetActivation() const { return activated; }
  bool GetDefaultActivation() const { return defaultActivation; }
  const olc::vi2d& GetPos() const { return pos; }
  float GetSize() const { return size; }
  const Direction& GetFacing() const { return facing; }
  std::string GetTileInformation() const;

  bool CanReceiveFrom(Direction dir) const { return canReceive[dir]; }
  bool CanOutputTo(Direction dir) const { return canOutput[dir]; }

  virtual std::string_view TileTypeName() const = 0;
  virtual bool IsEmitter() const = 0;
  virtual bool IsDeterministic() const = 0;
  virtual int GetTileTypeId() const = 0;

  // Virtual clone method for copying tiles
  virtual std::unique_ptr<GridTile> Clone() const = 0;  std::array<char, GRIDTILE_BYTESIZE> Serialize();
  static std::unique_ptr<GridTile> Deserialize(
      std::array<char, GRIDTILE_BYTESIZE> data);
  // Helper function to get the triangle points
  static std::array<olc::vf2d, 3> GetTrianglePoints(olc::vf2d screenPos,
                                                    int screenSize,
                                                    Direction facing);

 protected:
  // Convert between world and tile-relative coordinate systems
  Direction WorldToTileDirection(Direction worldDir) const;
  Direction TileToWorldDirection(Direction tileDir) const;

 private:
};

// This is a mere overlay of GridTile, and just implements the functions typical
// for a deterministic tile. It is used to mark tiles that are deterministic
// and can be used in deterministic simulations.
class DeterministicTile : public GridTile {
 public:
  explicit DeterministicTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f),
                    Direction facing = Direction::Top, float size = 1.0f,
                    bool defaultActivation = false,
                    olc::Pixel inactiveColor = olc::BLACK,
                    olc::Pixel activeColor = olc::BLACK)
      : GridTile(pos, facing, size, defaultActivation, inactiveColor,
                 activeColor) {}

  bool IsDeterministic() const override { return true; }
  bool IsEmitter() const override { return false; }
};

class LogicTile : public GridTile {
 public:
  explicit LogicTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f),
            Direction facing = Direction::Top, float size = 1.0f,
            bool defaultActivation = false,
            olc::Pixel inactiveColor = olc::BLACK,
            olc::Pixel activeColor = olc::BLACK)
      : GridTile(pos, facing, size, defaultActivation, inactiveColor,
                 activeColor) {}
  bool IsDeterministic() const override { return false; }
  std::vector<SignalEvent> PreprocessSignal(
      [[maybe_unused]] const SignalEvent incomingSignal) override {
    throw std::runtime_error(std::format(
        "Preprocessing is not supported for a Logic Tile of type {}",
        TileTypeName()));
  }
};

}  // namespace ElecSim
