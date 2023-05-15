#define OLC_PGE_APPLICATION

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "Tile.h"
#include "olcPixelGameEngine.h"

enum class TileType { Empty, Wire, ChargedWire };

// A tile. Can manage drawing and simulation itself.
class OldTile {
 private:
  struct Neighbour {
    TileSide side;
    std::shared_ptr<OldTile> tile;
  };
  std::vector<Neighbour> neighbours;

  olc::vi2d pos;
  olc::vi2d size;

  TileType type;
  TileType prevType;
  TileType nextType;
  TileSide ignoredSides;
  bool changed;

  static olc::Pixel getTileColor(TileType type) {
    switch (type) {
      case TileType::Wire:
        return olc::WHITE;
      case TileType::ChargedWire:
        return olc::YELLOW;
      case TileType::Empty:
      default:
        return olc::BLUE;
    }
  }

 public:
  OldTile() {
    pos = {0, 0};
    size = {0, 0};
    type = TileType::Empty;
    prevType = TileType::Empty;
    nextType = TileType::Empty;
    ignoredSides = TileSide::Empty;
  }

  OldTile(int x, int y, int size, TileType type) {
    pos = {x, y};
    this->size = {size, size};
    this->type = type;
    nextType = type;
    prevType = type;
    ignoredSides = TileSide::Empty;
  }

  ~OldTile() {}
  olc::vi2d Pos() { return pos; }
  int32_t Size() { return size.x; }
  TileType Type() { return type; }
  TileType PrevType() { return prevType; }
  TileType NextType() { return nextType; }

  void SetTileType(TileType newType, TileSide recvSide = TileSide::Empty) {
    if (!changed) {
      prevType = type;
      type = nextType;
      nextType = newType;
      changed = true;
    }
  }

  void SetNeighbour(TileSide side, std::shared_ptr<OldTile> tile) {
    neighbours.push_back((Neighbour){.side = side, .tile = tile});
  }

  void Draw(olc::PixelGameEngine* renderer) {
    renderer->FillRect(pos, size, getTileColor(type));
  }
  void Highlight(olc::PixelGameEngine* renderer) {
    renderer->DrawRect(pos, size - (olc::vi2d){1, 1}, olc::RED);
  }

  void Update() {
    if (nextType != TileType::Empty) {
      prevType = type;
      type = nextType;
    }
    changed = false;
  }

  void Simulate() {
    // Apply previous tick

    // If we are a normal wire, check around the neighbours for updates
    switch (type) {
      case TileType::ChargedWire:
        for (auto& neighbour : neighbours) {
          auto tile = neighbour.tile;
          if (tile->PrevType() != TileType::ChargedWire &&
              tile->Type() == TileType::Wire) {
            tile->SetTileType(TileType::ChargedWire);
          }
          SetTileType(TileType::Wire);
        }
        break;
      default:
        break;
    }
  }
};

#define TILE_SIZE 16
class Wire : public Tile {
 private:
  olc::Pixel unpulsedColor = olc::WHITE;
  olc::Pixel pulsedColor = olc::YELLOW;
  olc::Pixel highlightColor = olc::RED;

 public:
  Wire() {
    vPos = olc::vi2d(0, 0);
    uPixelSize = TILE_SIZE;
    wasPulsed = false;
    pulsed = false;
    std::cout << "Creating new wire at " << vPos / uPixelSize << std::endl;
  }

  Wire(uint32_t x, uint32_t y) {
    vPos = olc::vi2d(x, y);
    uPixelSize = TILE_SIZE;
    wasPulsed = false;
    pulsed = false;
    std::cout << "Creating new wire at " << vPos / uPixelSize << std::endl;
  }

  Wire(olc::vi2d pos) {
    vPos = pos;
    uPixelSize = TILE_SIZE;
    wasPulsed = false;
    pulsed = false;
    std::cout << "Creating new wire at " << vPos / uPixelSize << std::endl;
  }

  void Draw(olc::PixelGameEngine* renderer, bool highlighted = false) override {
    auto fillColor = pulsed ? pulsedColor : unpulsedColor;
    // std::cout << "Rendering " << pulsed << " at " << vPos / uPixelSize
    //           << std::endl;
    renderer->FillRect(vPos, olc::vi2d(uPixelSize, uPixelSize), fillColor);
    if (highlighted) {
      renderer->DrawRect(vPos, olc::vi2d(uPixelSize - 1, uPixelSize - 1),
                         highlightColor);
    }
  }

  void Simulate() override {
    if (true) {
      for (auto& rawNeighbour : neighbours) {
        if (rawNeighbour.expired()) {
          continue;
        }
        auto neighbour = rawNeighbour.lock();
        neighbour->Update(true);
      }
    }
  }

  ~Wire() {}
};

class EmptyTile : public Tile {
 private:
  olc::Pixel color = olc::BLUE;
  olc::Pixel highlightColor = olc::RED;

 public:
  EmptyTile() {
    vPos = olc::vi2d(0, 0);
    uPixelSize = TILE_SIZE;
    wasPulsed = false;
    pulsed = false;
  }
  EmptyTile(uint32_t x, uint32_t y) {
    vPos = olc::vi2d(x, y);
    uPixelSize = TILE_SIZE;
    wasPulsed = false;
    pulsed = false;
  }

  EmptyTile(olc::vi2d pos) {
    vPos = pos;
    uPixelSize = TILE_SIZE;
    wasPulsed = false;
    pulsed = false;
  }

  void Draw(olc::PixelGameEngine* renderer, bool highlighted = false) override {
    renderer->FillRect(vPos, olc::vi2d(uPixelSize, uPixelSize), color);
    if (highlighted) {
      renderer->DrawRect(vPos, olc::vi2d(uPixelSize - 1, uPixelSize - 1),
                         highlightColor);
    }
  }

  // Nothing should ever happen
  void Simulate() override { return; }

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

  bool OnUserCreate() override {
    tileDimensions = {ScreenWidth() / iTileSize, ScreenHeight() / iTileSize};
    std::cout << tileDimensions << std::endl;
    int tileCount = tileDimensions.x * tileDimensions.y;
    for (int y = 0; y < ScreenHeight(); y += 16) {
      for (int x = 0; x < ScreenWidth(); x += 16) {
        tiles.push_back(std::make_shared<EmptyTile>(x, y));
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

    // Modify tile if it's left-clicked
    auto& oldTile = tiles[index];
    if (GetMouse(0).bHeld) {
      tiles[index] = oldTile->Convert<Wire>();
    }
    if (GetMouse(1).bPressed) {
      tiles[index]->Update(true);
    }
    if (GetMouse(2).bPressed) {
      tiles[index] = oldTile->Convert<EmptyTile>();
    }

    if (frameCounter % 10 == 0) {
      for (auto& tile : tiles) {
        tile->Simulate();
      }
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