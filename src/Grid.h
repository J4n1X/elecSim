#pragma once

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "olcPixelGameEngine.h"

#define TILEUPDATESIDE_COUNT 4
enum class TileUpdateSide {
  Top = 1 << 0,
  Right = 1 << 1,
  Bottom = 1 << 2,
  Left = 1 << 3
};

TileUpdateSide FlipSide(TileUpdateSide side);
constexpr std::string_view SideName(TileUpdateSide side);

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
};

// A Grid Tile can draw itself, and it should implement a "Simulate" function,
// which checks the neighbours and then acts based on the input and returns
// the state it will have on the next tick.
class GridTile : public std::enable_shared_from_this<GridTile> {
 protected:
  olc::vi2d pos;
  int size;
  bool activated;
  bool defaultActivation;
  olc::Pixel inactiveColor;
  olc::Pixel activeColor;

  // Directions from which the tile can accept input
  TileUpdateFlags inputSides;
  // Directions in which the tile can output
  TileUpdateFlags outputSides;

 public:
  GridTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f), int size = 0,
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
  void SetSize(int newSize) { size = newSize; }
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
  int GetSize() { return size; }
  TileUpdateFlags GetInputSides() { return inputSides; }
  TileUpdateFlags GetOutputSides() { return outputSides; }

  // Constexpr Getters for compile time constants
  virtual constexpr std::string_view TileTypeName() = 0;
  virtual constexpr bool IsEmitter() = 0;
};

class EmptyGridTile : public GridTile {
 public:
  EmptyGridTile(olc::vi2d pos, int size)
      : GridTile(pos, size, false, olc::BLUE, olc::BLUE) {}
  ~EmptyGridTile(){};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override {
    (void)activatorSides;
    return TileUpdateFlags();
  };
};

class WireGridTile : public GridTile {
 public:
  WireGridTile(olc::vi2d pos, int size)
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
  EmitterGridTile(olc::vi2d pos, int size)
      : GridTile(pos, size, false, olc::DARK_CYAN, olc::CYAN, TileUpdateFlags(),
                 TileUpdateFlags::All()) {}
  EmitterGridTile() : EmitterGridTile(olc::vi2d(0, 0), 0) {}
  ~EmitterGridTile(){};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;

  constexpr std::string_view TileTypeName() override { return "Emitter"; }
  constexpr bool IsEmitter() override { return true; }
};

// The entire grid. Handles drawing the playfield, gathering the neighbours for
// simulation and much more.

// using GridPalette = BrushFactory<GridTile>;
class Grid {
 private:
  typedef std::map<olc::vi2d, std::shared_ptr<GridTile>> TileField;

  olc::Pixel backgroundColor = olc::BLUE;
  olc::Pixel highlightColor = olc::RED;
  olc::Pixel gridColor = olc::GREY;
  olc::vi2d renderWindow;

  // std::shared_ptr<GridPalette> palette;

  int uiLayer;
  int gameLayer;

  TileField tiles;
  std::vector<std::weak_ptr<GridTile>> alwaysUpdate;
  std::map<olc::vi2d, TileUpdateFlags> updates;

  // render scale in pixels
  uint32_t renderScale;
  // render offset in pixels
  olc::vi2d renderOffset;

 public:
  Grid(olc::vi2d size, uint32_t renderScale, olc::vi2d renderOffset,
       int uiLayer, int gameLayer);
  // Grid(const Grid& grid);
  Grid(int size_x, int size_y, uint32_t renderScale, olc::vi2d renderOffset,
       int uiLayer, int gameLayer)
      : Grid(olc::vi2d(size_x, size_y), renderScale, renderOffset, uiLayer,
             gameLayer) {}
  Grid() : Grid(0, 0, 0, olc::vi2d(0, 0), 0, 0){};
  ~Grid() {}
  olc::vi2d TranslateIndex(olc::vi2d index, TileUpdateSide side);

  // Utility functions
  olc::vi2d WorldToScreen(const olc::vf2d& pos);

  olc::vi2d ScreenToWorld(const olc::vi2d& pos);

  // Game Logic functions
  void Draw(olc::PixelGameEngine* renderer, olc::vi2d* highlightIndex);
  void Simulate();
  void ResetSimulation();

  // Setters
  void Resize(olc::vi2d newSize) { renderWindow = newSize; }
  void Resize(int newWidth, int newHeight) {
    Resize(olc::vi2d(newWidth, newHeight));
  }
  void SetRenderOffset(olc::vi2d newOffset) { renderOffset = newOffset; }
  void SetRenderScale(uint32_t newScale) { renderScale = newScale; }
  void EraseTile(olc::vi2d pos) { tiles.erase(pos); }
  void EraseTile(int x, int y) { EraseTile(olc::vi2d(x, y)); }
  template <typename T>
  void SetTile(olc::vi2d pos, std::shared_ptr<T> tile, bool emitter) {
    tiles.insert_or_assign(pos, tile);
    if (emitter) {
      // add it to the list of tiles which will always be updated
      alwaysUpdate.push_back(tile);
    }
    return;
  }
  template <typename T>
  void SetTile(int x, int y, std::shared_ptr<T> tile, bool emitter = false) {
    SetTile(tile, olc::vf2d(x, y));
  }

  // Getters
  olc::vi2d const& GetRenderWindow() { return renderWindow; }
  olc::vi2d const& GetRenderOffset() { return renderOffset; }
  uint32_t GetRenderScale() { return renderScale; }
  std::optional<std::shared_ptr<GridTile> const> GetTile(olc::vi2d pos) {
    auto tileIt = tiles.find(pos);
    return tileIt != tiles.end() ? std::optional{tileIt->second} : std::nullopt;
  }
  std::optional<std::shared_ptr<GridTile> const> GetTile(int x, int y) {
    return GetTile(olc::vi2d(x, y));
  }
};