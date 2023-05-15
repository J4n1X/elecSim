#include "Tile.h"

#include <iostream>

olc::vi2d Tile::SetPos(olc::vi2d pos) {
  this->vPos = pos;
  return this->vPos;
}

olc::vi2d Tile::SetPos(uint32_t x, uint32_t y) {
  vPos = olc::vi2d(x, y);
  return vPos;
}

uint32_t Tile::SetSize(uint32_t size) {
  uPixelSize = size;
  return uPixelSize;
}

void Tile::SetNeighbour(std::shared_ptr<Tile> neighbour, TileSide side) {
  neighbours[side] = neighbour;
}

olc::vi2d Tile::GetPos() { return vPos; }

uint32_t Tile::GetSize() { return uPixelSize; }

void Tile::Update(bool newPulseState) {
  if (!pulsed) {
    std::cout << "Me at " << vPos / uPixelSize
              << ": Received update, setting to " << newPulseState << std::endl;
  }
  wasPulsed = pulsed;
  pulsed = newPulseState;
}
bool Tile::IsPulsed() { return pulsed; }
bool Tile::WasPulsed() { return wasPulsed; }