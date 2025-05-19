#pragma once

#include <memory>
#include <vector>

#include "olcPixelGameEngine.h"

#define TILEUPDATESIDE_COUNT 4
enum class TileFacingSide : uint8_t {
  Top = 1 << 0,
  Right = 1 << 1,
  Bottom = 1 << 2,
  Left = 1 << 3
};

// TODO: Maybe rewrite this to just use a fixed array of bools?
struct TileUpdateFlags {
 private:
  uint8_t flagValue;

 public:
  TileUpdateFlags(uint8_t val) : flagValue(val) {}
  TileUpdateFlags(TileFacingSide val) : TileUpdateFlags((uint8_t)val) {}
  TileUpdateFlags() : TileUpdateFlags(0) {}
  inline void SetFlag(TileFacingSide flag, bool val) {
    if (val) {
      flagValue |= static_cast<uint8_t>(flag);
    } else {
      flagValue ^= static_cast<uint8_t>(flag);
    }
  }
  inline void FlipFlag(TileFacingSide flag) {
    flagValue ^= (flagValue & static_cast<uint8_t>(flag));
  }
  inline bool GetFlag(TileFacingSide flag) {
    return static_cast<bool>(flagValue & static_cast<uint8_t>(flag));
  }
  inline bool IsEmpty() { return flagValue == 0; }
  std::vector<TileFacingSide> GetFlags();

  constexpr static std::string_view GetFacingName(TileFacingSide side) {
    switch (side) {
      case TileFacingSide::Top:
        return "Top";
      case TileFacingSide::Bottom:
        return "Bottom";
      case TileFacingSide::Left:
        return "Left";
      case TileFacingSide::Right:
        return "Right";
      default:
        throw "Unknown";
    }
  }
  static TileUpdateFlags All() { return TileUpdateFlags(255); }
  static TileFacingSide RotateToFacing(const TileFacingSide& side,
                                       const TileFacingSide& facing);
  static TileFacingSide FlipSide(const TileFacingSide& side);
  static std::string_view SideName(const TileFacingSide& side);
};

#define GRIDTILE_BYTESIZE 13
// A Grid Tile can draw itself, and it should implement a "Simulate" function,
// which checks the neighbours and then acts based on the input and returns
// the state it will have on the next tick.
class GridTile : public std::enable_shared_from_this<GridTile> {
 protected:
  olc::vi2d pos;
  TileFacingSide facing;
  float size;
  bool activated;
  bool defaultActivation;
  olc::Pixel inactiveColor;
  olc::Pixel activeColor;

  // Directions from which the tile can accept input
  TileUpdateFlags inputSides;
  // Directions in which the tile can output
  TileUpdateFlags outputSides;

 public:
  GridTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f),
           TileFacingSide facing = TileFacingSide::Top, float size = 1.0f,
           bool defaultActivation = false,
           olc::Pixel inactiveColor = olc::BLACK,
           olc::Pixel activeColor = olc::BLACK,
           TileUpdateFlags inputSides = TileUpdateFlags(),
           TileUpdateFlags outputSides = TileUpdateFlags())
      : pos(pos),
        facing(facing),
        size(size),
        activated(defaultActivation),
        defaultActivation(defaultActivation),
        inactiveColor(inactiveColor),
        activeColor(activeColor),
        inputSides(inputSides),
        outputSides(outputSides) {}
  virtual void Draw(olc::PixelGameEngine* renderer, olc::vi2d screenPos,
                    int screenSize);
  virtual TileUpdateFlags Simulate(TileUpdateFlags activatorSides) = 0;
  // This function triggers when the user interacts with the tile while the
  // simulation is running. It should return the side from which it should be
  // activated.
  virtual TileUpdateFlags Interact() { return TileUpdateFlags(); };
  virtual ~GridTile() {};

  // Setters
  void SetPos(olc::vi2d newPos) { pos = newPos; }
  void SetFacing(TileFacingSide newFacing) { facing = newFacing; }
  void SetSize(float newSize) { size = newSize; }
  void SetActivation(bool newActivation) { activated = newActivation; }
  void SetInputSides(TileUpdateFlags newSides) { inputSides = newSides; }
  void SetOutputSides(TileUpdateFlags newSides) { inputSides = newSides; }

  // TODO: Should this be virtual? Sometimes, we don't want the user to be able
  // to change the default activation...
  void SetDefaultActivation(bool newDefault) { defaultActivation = newDefault; }
  void ResetActivation() { activated = defaultActivation; }

  // Getters for dynamic variables
  bool GetActivation() { return activated; }
  bool GetDefaultActivation() { return defaultActivation; }
  olc::vi2d GetPos() { return pos; }
  float GetSize() { return size; }
  // TODO: Do we still need those? Probably not, get rid of them.
  virtual TileUpdateFlags GetInputSides() { return inputSides; }
  virtual TileUpdateFlags GetOutputSides() { return outputSides; }

  // Save/load functions
  std::array<char, GRIDTILE_BYTESIZE> Serialize();
  static std::shared_ptr<GridTile> Deserialize(
      std::array<char, GRIDTILE_BYTESIZE> data);

  // Constexpr Getters for compile time constants
  virtual constexpr std::string_view TileTypeName() = 0;
  virtual constexpr bool IsEmitter() = 0;
  virtual constexpr int GetTileId() = 0;
};

/* class EmptyGridTile : public GridTile {
 public:
  EmptyGridTile(olc::vi2d pos, TileFacingSide facing, float size)
      : GridTile(pos, facing, size, false, olc::BLUE, olc::BLUE) {}
  ~EmptyGridTile() {};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override {
    (void)activatorSides;
    return TileUpdateFlags();
  };
}; */

// Transports a signal to the next tile in the direction of the facing
class WireGridTile : public GridTile {
 public:
  WireGridTile(olc::vi2d pos, TileFacingSide facing, float size)
      : GridTile(pos, facing, size, false, olc::WHITE, olc::YELLOW,
                 TileUpdateFlags::All(), TileUpdateFlags::All()) {}
  WireGridTile()
      : WireGridTile(olc::vi2d(0, 0), TileFacingSide::Top,
                     0) { /* std::cout << "Wire created." << std::endl; */ }
  ~WireGridTile() { /* std::cout << "Wire destroyed." << std::endl; */ };
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;

  constexpr std::string_view TileTypeName() override { return "Wire"; }
  constexpr bool IsEmitter() override { return false; }
  constexpr int GetTileId() override { return 0; }
};

// Spreads the signal to all sides excluding the one it came from
class JunctionGridTile : public GridTile {
 public:
  JunctionGridTile(olc::vi2d pos, TileFacingSide facing, float size)
      : GridTile(pos, facing, size, false, olc::WHITE, olc::YELLOW,
                 TileUpdateFlags::All(), TileUpdateFlags::All()) {}
  JunctionGridTile()
      : JunctionGridTile(
            olc::vi2d(0, 0), TileFacingSide::Top,
            0) { /* std::cout << "Junction created." << std::endl; */ }
  ~JunctionGridTile() { /* std::cout << "Junction destroyed." << std::endl; */
  };
  virtual void Draw(olc::PixelGameEngine* renderer, olc::vi2d screenPos,
                    int screenSize) override;
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;

  constexpr std::string_view TileTypeName() override { return "Junction"; }
  constexpr bool IsEmitter() override { return false; }
  constexpr int GetTileId() override { return 1; }
};

// Emits a signal every second tick
class EmitterGridTile : public GridTile {
 private:
  bool enabled = true;

 public:
  EmitterGridTile(olc::vi2d pos, TileFacingSide facing, float size)
      : GridTile(pos, facing, size, false, olc::DARK_CYAN, olc::CYAN,
                 TileUpdateFlags(), TileUpdateFlags::All()) {}
  EmitterGridTile()
      : EmitterGridTile(olc::vi2d(0, 0), TileFacingSide::Top,
                        0) { /* std::cout << "Emitter created." << std::endl; */
  }
  ~EmitterGridTile() { /* std::cout << "Emitter destroyed." << std::endl; */ };
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;
  TileUpdateFlags Interact() override;

  constexpr std::string_view TileTypeName() override { return "Emitter"; }
  constexpr bool IsEmitter() override { return true; }
  constexpr int GetTileId() override { return 2; }
};

// Requires an input from the side to activate, at that point, if an input comes
// from the bottom, it will emit.
class SemiConductorGridTile : public GridTile {
 public:
  SemiConductorGridTile(olc::vi2d pos, TileFacingSide facing, float size)
      : GridTile(pos, facing, size, false, olc::DARK_GREEN, olc::GREEN,
                 TileUpdateFlags(TileFacingSide::Bottom),
                 TileUpdateFlags(TileFacingSide::Top)) {}
  SemiConductorGridTile()
      : SemiConductorGridTile(
            olc::vi2d(0, 0), TileFacingSide::Top,
            0) { /* std::cout << "SemiConductor created." << std::endl; */ }
  ~SemiConductorGridTile() {
    /* std::cout << "SemiConductor destroyed." << std::endl; */
  };
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;
  TileUpdateFlags Interact() override;

  constexpr std::string_view TileTypeName() override { return "SemiConductor"; }
  constexpr bool IsEmitter() override { return false; }
  constexpr int GetTileId() override { return 3; }
};

// A button. Simple as that
class ButtonGridTile : public GridTile {
 public:
  ButtonGridTile(olc::vi2d pos, TileFacingSide facing, float size)
      : GridTile(pos, facing, size, false, olc::DARK_RED, olc::RED,
                 TileUpdateFlags(), TileUpdateFlags()) {}
  ButtonGridTile()
      : ButtonGridTile(olc::vi2d(0, 0), TileFacingSide::Top,
                       0) { /* std::cout << "Button created." << std::endl; */ }
  ~ButtonGridTile() { /* std::cout << "Button destroyed." << std::endl; */ };
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;
  TileUpdateFlags Interact() override;

  constexpr std::string_view TileTypeName() override { return "Button"; }
  constexpr bool IsEmitter() override { return false; }
  constexpr int GetTileId() override { return 4; }
};