#include "Tile.h"

#include <iostream>

Tile::TileUpdate::TileUpdate(std::shared_ptr<Tile> target, bool pulseState) {
  this->target = target;
  this->pulseState = pulseState;
};

olc::vi2d Tile::SetPos(olc::vi2d pos) {
  this->pos = pos;
  return this->pos;
}

olc::vi2d Tile::SetPos(uint32_t x, uint32_t y) {
  pos = olc::vi2d(x, y);
  return pos;
}

uint32_t Tile::SetSize(uint32_t size) {
  pixelSize = size;
  return pixelSize;
}

void Tile::SetNeighbour(std::shared_ptr<Tile> neighbour, TileSide side) {
  neighbours[side] = neighbour;
}

std::vector<std::shared_ptr<Tile>> Tile::GetNeighbours() {
  std::vector<std::shared_ptr<Tile>> ret;
  std::copy_if(neighbours.begin(), neighbours.end(), std::back_inserter(ret),
               [](auto n) { return n != nullptr; });
  return ret;
}

olc::vi2d Tile::GetPos() { return pos; }

uint32_t Tile::GetSize() { return pixelSize; }

void Tile::SetPulseState(bool newPulseState) {
  if (dynamic) pulsed = newPulseState;
}
bool Tile::IsPulsed() { return pulsed; }

bool Tile::IsDynamic() { return dynamic; }