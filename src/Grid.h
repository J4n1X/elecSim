#pragma once

#include <memory>
#include <set>
#include <vector>

#include "olcPixelGameEngine.h"

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
  TileUpdateFlags() : TileUpdateFlags(0) {}
  inline void SetFlag(TileUpdateSide flag) {
    flagValue |= static_cast<uint8_t>(flag);
  }
  inline void FlipFlag(TileUpdateSide flag) {
    flagValue ^= static_cast<uint8_t>(flag);
  }
  inline bool GetFlag(TileUpdateSide flag) {
    return static_cast<bool>(flagValue & static_cast<uint8_t>(flag));
  }
  std::vector<TileUpdateSide> GetFlags();
};

// A Grid Tile can draw itself, and it should implement a "Simulate" function,
// which checks the neighbours and then acts based on the input and returns
// the state it will have on the next tick.
class GridTile : public std::enable_shared_from_this<GridTile> {
 protected:
  bool activated = false;
  olc::vi2d pos = olc::vi2d(0, 0);
  size_t size = 0;

 public:
  virtual void Draw(olc::PixelGameEngine& renderer) = 0;
  virtual bool Simulate(TileUpdateFlags activeSides) = 0;
  virtual ~GridTile() = 0;
  inline void SetState(bool activated) { this->activated = activated; }
  inline bool Activated() { return activated; }
};

class EmptyGridTile : public GridTile {
 private:
  olc::Pixel color = olc::BLUE;

 public:
  EmptyGridTile(olc::vi2d pos, size_t size);
  void Draw(olc::PixelGameEngine& renderer) override;
  bool Simulate(TileUpdateFlags activeSides) override;
};

struct GridTileUpdate {
 public:
  olc::vi2d targetIndex;
  bool pulseState;
  GridTileUpdate(olc::vi2d targetIndex, bool pulseState)
      : targetIndex(targetIndex), pulseState(pulseState){};
};

// The entire grid. Handles drawing the playfield, gathering the neighbours for
// simulation and much more.
class Grid {
 private:
  olc::vi2d dimensions;
  size_t tileSize;
  typedef std::vector<std::vector<std::unique_ptr<GridTile>>> TileField;
  TileField tiles;
  std::set<GridTileUpdate> updates;

 public:
  Grid(olc::vi2d size, size_t tileSize);
  Grid(size_t size_x, size_t size_y, size_t tileSize)
      : Grid(olc::vi2d(size_x, size_y), tileSize) {}
  void Draw(olc::PixelGameEngine& renderer);
  void Simulate();

  template <typename T>
  void SetTile(olc::vi2d index, T tile) {
    auto ptrTile = std::make_unique<GridTile>(tile);
    tiles[index.y][index.x] = ptrTile;
  }
};