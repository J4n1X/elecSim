#pragma once

#include <memory>
#include <vector>

#include "olcPixelGameEngine.h"

#define TILEUPDATESIDE_COUNT 4
enum class TileUpdateSide {
  Top = 1 << 0,
  Right = 1 << 1,
  Bottom = 1 << 2,
  Left = 1 << 3
};

struct TileUpdateFlags {
 private:
  uint8_t flagValue;

 public:
  TileUpdateFlags(uint8_t val) : flagValue(val) {}
  TileUpdateFlags(TileUpdateSide val) : TileUpdateFlags((uint8_t)val) {}
  TileUpdateFlags() : TileUpdateFlags(0) {}
  inline void SetFlag(TileUpdateSide flag, bool val) {
    if (val) {
      flagValue |= static_cast<uint8_t>(flag);
    } else {
      flagValue ^= static_cast<uint8_t>(flag);
    }
  }
  inline void FlipFlag(TileUpdateSide flag) {
    flagValue ^= (flagValue & static_cast<uint8_t>(flag));
  }
  inline bool GetFlag(TileUpdateSide flag) {
    return static_cast<bool>(flagValue & static_cast<uint8_t>(flag));
  }
  inline bool IsEmpty() { return flagValue == 0; }
  std::vector<TileUpdateSide> GetFlags();

  static TileUpdateFlags All() { return TileUpdateFlags(255); }
  static TileUpdateSide FlipSide(const TileUpdateSide& side);
  static std::string_view SideName(const TileUpdateSide& side);
};

// A Grid Tile can draw itself, and it should implement a "Simulate" function,
// which checks the neighbours and then acts based on the input and returns
// the state it will have on the next tick.
class GridTile : public std::enable_shared_from_this<GridTile> {
 protected:
  olc::vi2d pos;
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
  GridTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f), float size = 0.0f,
           bool defaultActivation = false,
           olc::Pixel inactiveColor = olc::BLACK,
           olc::Pixel activeColor = olc::BLACK,
           TileUpdateFlags inputSides = TileUpdateFlags(),
           TileUpdateFlags outputSides = TileUpdateFlags())
      : pos(pos),
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
  virtual ~GridTile(){};

  // Setters
  void SetPos(olc::vi2d newPos) { pos = newPos; }
  void SetSize(float newSize) { size = newSize; }
  void SetActivation(bool newActivation) { activated = newActivation; }
  void SetInputSides(TileUpdateFlags newSides) { inputSides = newSides; }
  void SetOutputSides(TileUpdateFlags newSides) { inputSides = newSides; }
  // TODO: Should this be virtual? Sometimes, we don't want the user to be able
  // to change the default activation...
  void SetDefaultActivation(bool newDefault) { defaultActivation = newDefault; }
  void ResetActivation() { activated = defaultActivation; }

  // Getters for dynamic variables
  bool Activated() { return activated; }
  bool DefaultActivation() { return defaultActivation; }
  olc::vi2d GetPos() { return pos; }
  float GetSize() { return size; }
  TileUpdateFlags GetInputSides() { return inputSides; }
  TileUpdateFlags GetOutputSides() { return outputSides; }

  // Constexpr Getters for compile time constants
  virtual constexpr std::string_view TileTypeName() = 0;
  virtual constexpr bool IsEmitter() = 0;
};

class EmptyGridTile : public GridTile {
 public:
  EmptyGridTile(olc::vi2d pos, float size)
      : GridTile(pos, size, false, olc::BLUE, olc::BLUE) {}
  ~EmptyGridTile(){};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override {
    (void)activatorSides;
    return TileUpdateFlags();
  };
};

class WireGridTile : public GridTile {
 public:
  WireGridTile(olc::vi2d pos, float size)
      : GridTile(pos, size, false, olc::WHITE, olc::YELLOW,
                 TileUpdateFlags::All(), TileUpdateFlags::All()) {}
  WireGridTile() : WireGridTile(olc::vi2d(0, 0), 0) {}
  ~WireGridTile(){};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;

  constexpr std::string_view TileTypeName() override { return "Wire"; }
  constexpr bool IsEmitter() override { return false; }
};

class EmitterGridTile : public GridTile {
 public:
  EmitterGridTile(olc::vi2d pos, float size)
      : GridTile(pos, size, false, olc::DARK_CYAN, olc::CYAN, TileUpdateFlags(),
                 TileUpdateFlags::All()) {}
  EmitterGridTile() : EmitterGridTile(olc::vi2d(0, 0), 0) {}
  ~EmitterGridTile(){};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;

  constexpr std::string_view TileTypeName() override { return "Emitter"; }
  constexpr bool IsEmitter() override { return true; }
};