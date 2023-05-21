#pragma once

#include <map>
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
  inline void SetFlag(TileUpdateSide flag) {
    flagValue |= static_cast<uint8_t>(flag);
  }
  inline void FlipFlag(TileUpdateSide flag) {
    flagValue ^= static_cast<uint8_t>(flag);
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
  olc::Pixel inactiveColor;
  olc::Pixel activeColor;
  olc::Pixel highlightColor;

 public:
  GridTile(olc::vi2d pos = olc::vi2d(0, 0), int size = 0,
           bool activated = false, olc::Pixel inactiveColor = olc::BLACK,
           olc::Pixel activeColor = olc::BLACK,
           olc::Pixel highlightColor = olc::BLACK)
      : pos(pos),
        size(size),
        activated(activated),
        inactiveColor(inactiveColor),
        activeColor(activeColor),
        highlightColor(highlightColor) {}
  virtual void Draw(olc::PixelGameEngine* renderer);
  virtual void Highlight(olc::PixelGameEngine* renderer);
  virtual TileUpdateFlags Simulate(TileUpdateFlags activatorSides) = 0;
  virtual ~GridTile(){};

  inline void SetState(bool activated) { this->activated = activated; }
  inline bool Activated() { return activated; }
  inline olc::vi2d GetPos() { return pos; }
  inline int GetSize() { return size; }
};

class EmptyGridTile : public GridTile {
 public:
  EmptyGridTile(olc::vi2d pos, int size)
      : GridTile(pos, size, false, olc::BLUE, olc::BLUE, olc::RED) {}
  ~EmptyGridTile(){};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override {
    return TileUpdateFlags();
  };
};

class WireGridTile : public GridTile {
 public:
  WireGridTile(olc::vi2d pos, int size)
      : GridTile(pos, size, false, olc::WHITE, olc::YELLOW, olc::RED) {}
  ~WireGridTile(){};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;
};

class EmitterGridTile : public GridTile {
 public:
  EmitterGridTile(olc::vi2d pos, int size)
      : GridTile(pos, size, false, olc::DARK_CYAN, olc::CYAN, olc::RED) {}
  ~EmitterGridTile(){};
  TileUpdateFlags Simulate(TileUpdateFlags activatorSides) override;
};

// The entire grid. Handles drawing the playfield, gathering the neighbours for
// simulation and much more.
class Grid {
  typedef std::vector<std::vector<std::unique_ptr<GridTile>>> TileField;

 private:
  olc::vi2d dimensions;
  int tileSize;
  TileField tiles;
  std::vector<olc::vi2d> alwaysUpdate;
  std::vector<olc::vi2d> drawQueue;
  std::map<olc::vi2d, TileUpdateFlags> updates;

 public:
  Grid(){};
  Grid(olc::vi2d size, int tileSize);
  Grid(int size_x, int size_y, int tileSize)
      : Grid(olc::vi2d(size_x, size_y), tileSize) {}
  void Draw(olc::PixelGameEngine* renderer, olc::vi2d* highlightIndex,
            uint32_t gameLayer, uint32_t uiLayer);
  void Simulate();

  inline std::unique_ptr<GridTile>& GetTile(int x, int y) {
    return tiles[y][x];
  }
  inline std::unique_ptr<GridTile>& GetTile(olc::vi2d pos) {
    return tiles[pos.y][pos.x];
  }

  template <typename T>
  void SetTile(std::unique_ptr<T> tile, olc::vi2d index, bool emitter = false) {
    if (index.y < tiles.size()) {
      if (index.x < tiles[index.y].size()) {
        tiles[index.y][index.x] = std::move(tile);
        auto it = std::find(alwaysUpdate.begin(), alwaysUpdate.end(), index);
        if (emitter) {
          alwaysUpdate.emplace(it, index);
        } else {
          if (it != alwaysUpdate.end()) {
            std::vector<olc::vi2d> newIndexes;
            std::copy_if(alwaysUpdate.begin(), alwaysUpdate.end(),
                         std::back_inserter(newIndexes),
                         [&](olc::vi2d i) { return i != *it; });

            // TODO: Any updates that were caused by this should be updated or
            // removed

            alwaysUpdate = newIndexes;
          }
        }
        drawQueue.push_back(index);
        return;
      }
    }
    throw "Tile Index out of bounds";
  }
  template <typename T>
  void SetTile(std::unique_ptr<T> tile, int x, int y, bool emitter = false) {
    SetTile(tile, olc::vi2d(x, y));
  }

  void TriggerUpdate(olc::vi2d pos) {
    updates.emplace(pos, TileUpdateFlags::All());
  }
};