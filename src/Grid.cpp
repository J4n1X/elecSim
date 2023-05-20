#include "Grid.h"

std::vector<TileUpdateSide> TileUpdateFlags::GetFlags() {
  std::vector<TileUpdateSide> ret;
  for (int i = 0; i < 4; i++) {
    auto flag = flagValue & (1 << i);
    ret.push_back(static_cast<TileUpdateSide>(flag));
  }
  return ret;
}

EmptyGridTile::EmptyGridTile(olc::vi2d pos, size_t size) {
  this->pos = pos;
  this->size = size;
}
void EmptyGridTile::Draw(olc::PixelGameEngine& renderer) {
  renderer.FillRect(pos, olc::vi2d(size, size), color);
}

bool EmptyGridTile::Simulate(TileUpdateFlags activeSides) { return false; }

Grid::Grid(olc::vi2d size, size_t tileSize) {
  dimensions = size;
  this->tileSize = tileSize;
  tiles.resize(dimensions.y);
  for (size_t y = 0; y < dimensions.y; y++) {
    tiles[y].resize(dimensions.x);
    for (size_t x = 0; x < dimensions.x; x++) {
      auto pos = olc::vi2d(x * tileSize, y * tileSize);
      tiles[y][x] = std::make_unique<EmptyGridTile>(pos, tileSize);
    }
  }
}

void Grid::Draw(olc::PixelGameEngine& renderer) {
  for (size_t y = 0; y < dimensions.y; y++) {
    for (size_t x = 0; x < dimensions.x; x++) {
      tiles[y][x]->Draw(renderer);
    }
  }
}
