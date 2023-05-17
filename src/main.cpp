#define OLC_PGE_APPLICATION

#include <cstddef>
#include <cstdio>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <vector>

#include "Tile.h"
#include "olcPixelGameEngine.h"

#define TILE_SIZE 16
class Wire : public Tile {
 private:
  olc::Pixel unpulsedColor = olc::WHITE;
  olc::Pixel pulsedColor = olc::YELLOW;
  olc::Pixel highlightColor = olc::RED;

 public:
  Wire(uint32_t x, uint32_t y) {
    pos = olc::vi2d(x, y);
    pixelSize = TILE_SIZE;
    dynamic = true;
  }

  Wire() : Wire(0, 0) {}
  Wire(olc::vi2d pos) : Wire(pos.x, pos.y) {}

  void Draw(olc::PixelGameEngine* renderer, bool highlighted = false) override {
    auto fillColor = pulsed ? pulsedColor : unpulsedColor;
    // std::cout << "Rendering " << pulsed << " at " << vPos / pixelSize
    //           << std::endl;
    renderer->FillRect(pos, olc::vi2d(pixelSize, pixelSize), fillColor);
    if (highlighted) {
      renderer->DrawRect(pos, olc::vi2d(pixelSize - 1, pixelSize - 1),
                         highlightColor);
    }
  }

  void Simulate(std::vector<TileUpdate>& updateQueue) override {
    // if (!pulsed) throw "Only pulsed wires should be simulated";

    for (auto& neighbour : GetNeighbours()) {
      // Pulse inactive, dynamic tiles
      if (!neighbour->IsPulsed() && neighbour->IsDynamic()) {
        updateQueue.push_back(TileUpdate(neighbour, true));
      }
    }
    // Wire can only stay active for one simulation tick, so disable on the next
    updateQueue.push_back(TileUpdate(shared_from_this(), false));
  }

  ~Wire() {}
};

class EmptyTile : public Tile {
 private:
  olc::Pixel color = olc::BLUE;
  olc::Pixel highlightColor = olc::RED;

 public:
  EmptyTile(uint32_t x, uint32_t y) {
    pos = olc::vi2d(x, y);
    pixelSize = TILE_SIZE;
    pulsed = false;
    dynamic = false;
  }
  EmptyTile() : EmptyTile(0, 0) {}
  EmptyTile(olc::vi2d pos) : EmptyTile(pos.x, pos.y) {}

  void Draw(olc::PixelGameEngine* renderer, bool highlighted = false) override {
    renderer->FillRect(pos, olc::vi2d(pixelSize, pixelSize), color);
    if (highlighted) {
      renderer->DrawRect(pos, olc::vi2d(pixelSize - 1, pixelSize - 1),
                         highlightColor);
    }
  }

  // Nothing should ever happen
  void Simulate(std::vector<TileUpdate>& updateQueue) override { return; }

  ~EmptyTile() {}
};

class Game : public olc::PixelGameEngine {
 public:
  Game() { sAppName = "Electricity Simulator"; }

  std::vector<std::shared_ptr<Tile>>& Tiles() { return tiles; }

 private:
  int iTileSize = 16;
  olc::vi2d tileDimensions;
  std::vector<std::shared_ptr<Tile>> tiles;
  std::vector<Tile::TileUpdate> updateQueue;

  bool OnUserCreate() override {
    tileDimensions = {ScreenWidth() / iTileSize, ScreenHeight() / iTileSize};
    std::cout << tileDimensions << std::endl;
    int tileCount = tileDimensions.x * tileDimensions.y;
    for (int y = 0; y < ScreenHeight(); y += 16) {
      for (int x = 0; x < ScreenWidth(); x += 16) {
        auto newTile = std::make_shared<EmptyTile>(x, y);
        tiles.push_back(newTile->shared_from_this());
      }
    }
    // Set neighbour pointers
    for (int y = 0; y < tileDimensions.y; y++) {
      for (int x = 0; x < tileDimensions.x; x++) {
        auto tile = tiles[y * tileDimensions.x + x];
        auto topIndex = (y - 1) * tileDimensions.x + x;
        auto bottomIndex = (y + 1) * tileDimensions.x + x;
        auto leftIndex = y * tileDimensions.x + x - 1;
        auto rightIndex = y * tileDimensions.x + x + 1;

        if (y - 1 >= 0) tile->SetNeighbour(tiles[topIndex], TileSide::Top);
        if (y + 1 < tileDimensions.y)
          tile->SetNeighbour(tiles[bottomIndex], TileSide::Bottom);
        if (x - 1 >= 0) tile->SetNeighbour(tiles[leftIndex], TileSide::Left);
        if (x + 1 < tileDimensions.x)
          tile->SetNeighbour(tiles[rightIndex], TileSide::Right);
      }
    }
    return true;
  }

  uint64_t frameCounter = 0;
  bool OnUserUpdate(float fElapsedTime) override {
    frameCounter++;

    // Highlight tile below mouse
    auto selTileXIndex = GetMouseX() / iTileSize;
    auto selTileYIndex = GetMouseY() / iTileSize;
    auto index = selTileYIndex * tileDimensions.x + selTileXIndex;

    for (int i = 0; i < tiles.size(); i++) {
      auto highlight = i == index;
      tiles[i]->Draw(this, highlight);
    }

    // DrawString(olc::vi2d(0, 0), "This is a test", olc::BLACK);

    if (frameCounter % 20 == 0) {
      std::set<std::shared_ptr<Tile>> candidates;
      // Apply changes
      for (auto& update : updateQueue) {
        if (update.pulseState) {
          candidates.emplace(update.target);
        }
      }

      std::vector<Tile::TileUpdate> newUpdates;
      // Run Updates
      for (auto& candidate : candidates) {
        if (candidate->IsDynamic()) {
          candidate->Simulate(newUpdates);
        }
      }
      for (auto& update : updateQueue) {
        update.target->SetPulseState(update.pulseState);
      }
      updateQueue = newUpdates;
    }

    // Modify tile if it's left-clicked
    auto& oldTile = tiles[index];
    if (GetMouse(0).bHeld) {
      tiles[index] = oldTile->Convert<Wire>();
    }
    if (GetMouse(1).bHeld) {
      updateQueue.push_back(Tile::TileUpdate(tiles[index], true));
    }
    if (GetMouse(2).bHeld) {
      tiles[index] = oldTile->Convert<EmptyTile>();
    }
    return true;
  }
};

int main(int arc, char** argv) {
  Game game = Game();
  if (game.Construct(640, 480, 2, 2, false, true)) {
    game.Start();
  }
}