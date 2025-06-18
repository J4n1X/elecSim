#pragma once

#include <array>
#include <string>
#include <vector>

#include "Common.h"
#include "v2d.h"

namespace ElecSim {

// Base tile class
// TODO: I want to add a Construct() function that creates a new tile of any
// type, maybe with templates? A factory?
// TODO: Maybe we should not store refNum in here, but this is the easiest way
// right now.
enum class TileType : int {
  Wire,
  Junction,
  Emitter,
  SemiConductor,
  Button,
  Inverter,
  Crossing
};
class GridTile : public std::enable_shared_from_this<GridTile> {
 protected:
  vi2d pos;
  Direction facing;
  bool activated;
  bool defaultActivation;
  TileSideStates canReceive;
  TileSideStates canOutput;
  TileSideStates inputStates;

 public:
  GridTile(vi2d pos = vi2d(0.0f, 0.0f), Direction facing = Direction::Top,
           bool defaultActivation = false);

  // Copy constructor
  GridTile(const GridTile& other);

  // Move constructor
  GridTile(GridTile&& other) noexcept;

  // Copy assignment operator
  GridTile& operator=(const GridTile& other);

  virtual ~GridTile() {};

  // Removed.
  // virtual void Draw(olc::PixelGameEngine* renderer, olc::vf2d screenPos,
  //                  float screenSize, int alpha = 255);

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

  void SetPos(vi2d newPos) { pos = newPos; }
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
  const vi2d GetPos() const { return pos; }
  const Direction& GetFacing() const { return facing; }
  std::string GetTileInformation() const;

  bool CanReceiveFrom(Direction dir) const { return canReceive[dir]; }
  bool CanOutputTo(Direction dir) const { return canOutput[dir]; }

  virtual constexpr const char* TileTypeName() const = 0;
  virtual bool IsEmitter() const = 0;
  virtual bool IsDeterministic() const = 0;
  virtual TileType GetTileType() const = 0;

  // Pure virtual clone method - must be implemented by all derived classes
  [[nodiscard]] virtual std::unique_ptr<GridTile> Clone() const = 0;

  std::array<char, GRIDTILE_BYTESIZE> Serialize();
  static std::unique_ptr<GridTile> Deserialize(
      std::array<char, GRIDTILE_BYTESIZE> data);

 protected:
  // Convert between world and tile-relative coordinate systems
  Direction WorldToTileDirection(Direction worldDir) const;
  Direction TileToWorldDirection(Direction tileDir) const;
};

// This is a mere overlay of GridTile, and just implements the functions typical
// for a deterministic tile. It is used to mark tiles that are deterministic
// and can be used in deterministic simulations.
class DeterministicTile : public GridTile {
 public:
  explicit DeterministicTile(vi2d pos = vi2d(0.0f, 0.0f),
                             Direction facing = Direction::Top,
                             bool defaultActivation = false)
      : GridTile(pos, facing, defaultActivation) {}

  bool IsDeterministic() const override { return true; }
  bool IsEmitter() const override { return false; }
};

class LogicTile : public GridTile {
 public:
  explicit LogicTile(vi2d pos = vi2d(0.0f, 0.0f),
                     Direction facing = Direction::Top,
                     bool defaultActivation = false)
      : GridTile(pos, facing, defaultActivation) {}
  bool IsDeterministic() const override { return false; }
  std::vector<SignalEvent> PreprocessSignal(
      [[maybe_unused]] const SignalEvent incomingSignal) override {
    throw std::runtime_error(std::format(
        "Preprocessing is not supported for a Logic Tile of type {}",
        TileTypeName()));
  }
};

}  // namespace ElecSim
