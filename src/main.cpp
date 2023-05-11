#define OLC_PGE_APPLICATION

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "olcPixelGameEngine.h"

enum class TileType { Empty, Wire, ChargedWire };
enum class TileSide { Empty = 0, Top = 1, Bottom = 2, Left = 4, Right = 16 };

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

// A tile. Can manage drawing and simulation itself.
class Tile {
 private:
  struct Neighbour {
    TileSide side;
    std::shared_ptr<Tile> tile;
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
  Tile() {
    pos = {0, 0};
    size = {0, 0};
    type = TileType::Empty;
    prevType = TileType::Empty;
    nextType = TileType::Empty;
    ignoredSides = TileSide::Empty;
  }

  Tile(int x, int y, int size, TileType type) {
    pos = {x, y};
    this->size = {size, size};
    this->type = type;
    nextType = type;
    prevType = type;
    ignoredSides = TileSide::Empty;
  }

  ~Tile() {}
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

  void SetNeighbour(TileSide side, std::shared_ptr<Tile> tile) {
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
        tiles.push_back(
            std::make_shared<Tile>(Tile(x, y, iTileSize, TileType::Empty)));
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

        if (y - 1 >= 0) tile->SetNeighbour(TileSide::Top, tiles[topIndex]);
        if (y + 1 < tileDimensions.y)
          tile->SetNeighbour(TileSide::Bottom, tiles[bottomIndex]);
        if (x - 1 >= 0) tile->SetNeighbour(TileSide::Left, tiles[leftIndex]);
        if (x + 1 < tileDimensions.x)
          tile->SetNeighbour(TileSide::Right, tiles[rightIndex]);
      }
    }
    return true;
  }

  uint64_t frameCounter = 0;
  bool OnUserUpdate(float fElapsedTime) override {
    frameCounter++;
    if (frameCounter % 10 == 0) {
      for (auto& tile : tiles) {
        tile->Update();
      }
    }

    for (auto& tile : this->tiles) {
      tile->Draw(this);
    }

    // Highlight tile below mouse
    auto selTileXIndex = GetMouseX() / iTileSize;
    auto selTileYIndex = GetMouseY() / iTileSize;
    auto index = selTileYIndex * tileDimensions.x + selTileXIndex;
    tiles[index]->Highlight(this);

    // Modify tile if it's left-clicked
    if (GetMouse(0).bHeld) {
      tiles[index]->SetTileType(TileType::Wire);
    }
    if (GetMouse(1).bPressed) {
      tiles[index]->SetTileType(TileType::ChargedWire);
    }
    if (GetMouse(2).bPressed) {
      tiles[index]->SetTileType(TileType::Empty);
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