#pragma once

#include <memory>

#include "olcPixelGameEngine.h"

enum TileSide : uint32_t {
  Top = 0,
  Bottom = 1,
  Left = 2,
  Right = 3,
  Empty = 4
};
constexpr TileSide flipSide(TileSide side) {
  switch (side) {
    case TileSide::Top:
      return TileSide::Bottom;
    case TileSide::Bottom:
      return TileSide::Top;
    case TileSide::Left:
      return TileSide::Right;
    case TileSide::Right:
      return TileSide::Left;
    default:
      return TileSide::Empty;
  }
}

class Tile : public std::enable_shared_from_this<Tile> {
 protected:
  olc::vi2d pos = {0, 0};
  uint32_t pixelSize = 0;
  // Whether the tile has been activated by a pulse
  bool pulsed = false;
  // Whether the tile accepts state changes
  bool dynamic = false;
  std::array<std::shared_ptr<Tile>, 4> neighbours = {};

 public:
  struct TileUpdate {
   public:
    std::shared_ptr<Tile> target;
    bool pulseState;
    TileUpdate(std::shared_ptr<Tile> target, bool pulseState);
  };

  virtual void Draw(olc::PixelGameEngine* renderer, bool highlighted) = 0;
  virtual void Simulate(std::vector<TileUpdate>& updateQueue) = 0;
  virtual ~Tile(){
      // std::cout << "Destroying Tile at " << vPos / uPixelSize << std::endl;
  };

  olc::vi2d SetPos(olc::vi2d pos);
  olc::vi2d SetPos(uint32_t x, uint32_t y);
  void SetNeighbour(std::shared_ptr<Tile> neighbour, TileSide side);
  std::vector<std::shared_ptr<Tile>> GetNeighbours();

  uint32_t SetSize(uint32_t size);
  olc::vi2d GetPos();
  uint32_t GetSize();

  template <typename T>
  std::shared_ptr<T> Convert() {
    auto ret = std::make_shared<T>(pos);
    for (uint32_t i = 0; i < neighbours.size(); i++) {
      auto neighbour = neighbours[i];
      if (neighbour == nullptr) {
        continue;
      }
      ret->SetNeighbour(neighbour, (TileSide)i);
      neighbour->SetNeighbour(ret, flipSide((TileSide)i));
    }
    return ret;
  }

  void SetPulseState(bool newPulseState);
  bool IsPulsed();
  bool IsDynamic();
};