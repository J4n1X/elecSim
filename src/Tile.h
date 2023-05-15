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
      return TileSide::Left;
    case TileSide::Right:
      return TileSide::Left;
    default:
      return TileSide::Empty;
  }
}

class Tile {
 protected:
  olc::vi2d vPos = {0, 0};
  uint32_t uPixelSize = 0;
  bool wasPulsed = false;
  bool pulsed = false;
  std::array<std::weak_ptr<Tile>, 4> neighbours;

 public:
  virtual void Draw(olc::PixelGameEngine* renderer, bool highlighted) = 0;
  virtual void Simulate() = 0;
  virtual ~Tile(){
      // std::cout << "Destroying Tile at " << vPos / uPixelSize << std::endl;
  };

  // Tile() {
  //   vPos = olc::vi2d(0, 0);
  //   uPixelSize = 0;
  // }

  olc::vi2d SetPos(olc::vi2d pos);
  olc::vi2d SetPos(uint32_t x, uint32_t y);
  void SetNeighbour(std::shared_ptr<Tile> neighbour, TileSide side);
  uint32_t SetSize(uint32_t size);
  olc::vi2d GetPos();
  uint32_t GetSize();
  //  std::vector<std::shared_ptr<Tile>> const& GetNeighbours();

  template <typename T>
  std::shared_ptr<T> Convert() {
    auto ret = std::make_shared<T>(vPos);
    for (uint32_t i = 0; i < neighbours.size(); i++) {
      if (neighbours[i].expired()) {
        continue;
      }
      std::cout << "Updating Neighbour" << i << std::endl;
      auto neighbour = neighbours[i].lock();
      ret->SetNeighbour(neighbour, flipSide((TileSide)i));
      neighbour->SetNeighbour(ret, (TileSide)i);
    }
    return ret;
  }

  void Update(bool newPulseState);
  bool IsPulsed();
  bool WasPulsed();
};