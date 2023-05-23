#pragma once

#include <map>
#include <memory>
#include <type_traits>
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
const char* SideName(TileUpdateSide side);

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
  bool defaultActivation = false;
  olc::Pixel inactiveColor;
  olc::Pixel activeColor;

 public:
  GridTile(olc::vi2d pos = olc::vf2d(0.0f, 0.0f), int size = 0,
           bool defaultActivation = false,
           olc::Pixel inactiveColor = olc::BLACK,
           olc::Pixel activeColor = olc::BLACK)
      : pos(pos),
        size(size),
        activated(defaultActivation),
        defaultActivation(defaultActivation),
        inactiveColor(inactiveColor),
        activeColor(activeColor) {}
  virtual void Draw(olc::PixelGameEngine* renderer);
  virtual TileUpdateFlags Simulate(TileUpdateFlags activatorSides) = 0;
  virtual ~GridTile(){};

  inline void SetActivation(bool activated) { this->activated = activated; }
  inline void SetDefaultActivation(bool defaultActivation) {
    this->defaultActivation = defaultActivation;
  }
  inline bool Activated() { return activated; }
  inline bool DefaultActivation() { return defaultActivation; }
  inline void Reset() { activated = defaultActivation; }
  inline olc::vi2d GetPos() { return pos; }
  inline int GetSize() { return size; }
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
  WireGridTile(){};
  WireGridTile(olc::vi2d pos, int size)
      : GridTile(pos, size, false, olc::WHITE, olc::YELLOW) {}
  ~WireGridTile() { std::cout << "Dropping Wire Tile" << std::endl; };
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;
};

class EmitterGridTile : public GridTile {
 public:
  EmitterGridTile(){};
  EmitterGridTile(olc::vi2d pos, int size)
      : GridTile(pos, size, false, olc::DARK_CYAN, olc::CYAN) {}
  ~EmitterGridTile() { std::cout << "Dropping Emitter Tile" << std::endl; };
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;
};

// The entire grid. Handles drawing the playfield, gathering the neighbours for
// simulation and much more.

// using GridPalette = BrushFactory<GridTile>;
class Grid {
  typedef std::map<olc::vi2d, std::shared_ptr<GridTile>> TileField;

 private:
  olc::Pixel backgroundColor = olc::BLUE;
  olc::Pixel highlightColor = olc::RED;
  olc::vi2d dimensions;

  // std::shared_ptr<GridPalette> palette;

  int uiLayer;
  int gameLayer;

  TileField tiles;
  std::vector<std::weak_ptr<GridTile>> alwaysUpdate;
  std::map<olc::vi2d, TileUpdateFlags> updates;

 public:
  static int worldScale;
  static olc::vi2d worldOffset;
  static olc::vi2d WorldToScreen(const olc::vi2d& pos) {
    auto x = (pos.x - worldOffset.x) * worldScale;
    auto y = (pos.y - worldOffset.y) * worldScale;
    return olc::vi2d(x, y);
  }

  static olc::vi2d ScreenToWorld(const olc::vi2d& pos) {
    auto x = pos.x / worldScale + worldOffset.x;
    auto y = pos.y / worldScale + worldOffset.y;
    return olc::vi2d(x, y);
  }

  Grid(olc::vi2d size, int uiLayer, int gameLayer);
  // Grid(const Grid& grid);
  Grid(int size_x, int size_y, int uiLayer, int gameLayer)
      : Grid(olc::vi2d(size_x, size_y), uiLayer, gameLayer) {}
  Grid() : Grid(0, 0, 0, 0){};
  olc::vi2d TranslateIndex(olc::vi2d index, TileUpdateSide side);

  // We want to give away full control over the Palette
  // std::shared_ptr<GridPalette> GetPalette() { return palette; }

  void Draw(olc::PixelGameEngine* renderer, olc::vi2d* highlightIndex);
  void Simulate();

  std::shared_ptr<GridTile> GetTile(olc::vi2d pos) {
    auto tileIt = tiles.find(pos);
    return tileIt != tiles.end() ? tileIt->second : nullptr;
  }
  std::shared_ptr<GridTile> GetTile(int x, int y) {
    return GetTile(olc::vi2d(x, y));
  }
  void EraseTile(olc::vi2d pos) { tiles.erase(pos); }
  void EraseTile(int x, int y) { EraseTile(olc::vi2d(x, y)); }
  void TriggerUpdate(olc::vf2d pos) {
    updates.emplace(pos, TileUpdateFlags::All());
  }

  template <typename T>
  void SetTile(olc::vi2d pos, std::shared_ptr<T> tile, bool emitter) {
    tiles.insert_or_assign(pos, tile);
    if (emitter) {
      // add it to the list of tiles which will always be updated
      alwaysUpdate.push_back(tile);
    }

    // Remove any pending updates
    updates.clear();
    // Reset the state of all tiles to the set default
    for ([[maybe_unused]] auto& [pos, tile] : tiles) {
      tile->Reset();
    }

    return;
  }
  template <typename T>
  void SetTile(int x, int y, std::unique_ptr<T> tile, bool emitter = false) {
    SetTile(tile, olc::vf2d(x, y));
  }
};