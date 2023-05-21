#define OLC_PGE_APPLICATION

#include <cstddef>
#include <cstdio>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <vector>

#include "Grid.h"
#include "olcPixelGameEngine.h"

#define TILE_SIZE 16

class Game : public olc::PixelGameEngine {
 public:
  Game() { sAppName = "Electricity Simulator"; }

 private:
  int iTileSize = 16;
  uint32_t gameLayer;
  uint32_t uiLayer;
  olc::vi2d tileDimensions;
  bool paused;
  float accumulatedTime;
  float updateInterval;
  Grid grid;

  bool OnUserCreate() override {
    tileDimensions = {ScreenWidth() / iTileSize, ScreenHeight() / iTileSize};
    std::cout << tileDimensions << std::endl;
    int tileCount = tileDimensions.x * tileDimensions.y;

    gameLayer = CreateLayer();
    uiLayer = 0;

    paused = false;
    accumulatedTime = 0.0f;
    updateInterval = 0.2f;

    EnableLayer(gameLayer, true);
    grid = Grid(ScreenWidth(), ScreenHeight(), TILE_SIZE);

    return true;
  }

  bool OnUserUpdate(float fElapsedTime) override {
    accumulatedTime += fElapsedTime;
    if (accumulatedTime > updateInterval && !paused) {
      grid.Simulate();
      accumulatedTime = 0;
    }

    // Highlight tile below mouse
    auto selTileXIndex = GetMouseX() / iTileSize;
    auto selTileYIndex = GetMouseY() / iTileSize;
    auto index = olc::vi2d(selTileXIndex, selTileYIndex);

    grid.Draw(this, &index, gameLayer, uiLayer);

    // Modify tile if it's left-clicked
    auto& tile = grid.GetTile(index);
    if (GetMouse(0).bHeld) {  // Create wire
      auto wire =
          std::make_unique<WireGridTile>(tile->GetPos(), tile->GetSize());
      grid.SetTile(std::move(wire), index, false);
    }
    if (GetMouse(1).bHeld) {  // Create pulse emitter
      auto emitter =
          std::make_unique<EmitterGridTile>(tile->GetPos(), tile->GetSize());
      grid.SetTile(std::move(emitter), index, true);
    }
    if (GetMouse(2).bHeld) {  // Remove tile
      auto emptyTile =
          std::make_unique<EmptyGridTile>(tile->GetPos(), tile->GetSize());
      grid.SetTile(std::move(emptyTile), index, false);
    }
    if (GetKey(olc::SPACE).bPressed) {
      paused = !paused;
    }
    if (GetKey(olc::COMMA).bPressed) {
      updateInterval += 0.05f;
    }
    if (GetKey(olc::PERIOD).bPressed) {
      if (updateInterval > 0.0f) {
        updateInterval -= 0.05f;
      } else {
        updateInterval = 0.0f;
      }
    }
    if (GetKey(olc::ESCAPE).bPressed) {
      return false;
    }

    std::stringstream ss;
    ss << "Selected: "
       << "TEMP; " << (paused ? "Paused" : "; Running") << '\n'
       << "Press '.' to increase and ',' to decrease speed";
    SetDrawTarget(uiLayer);
    DrawString(olc::vi2d(0, 0), ss.str(), olc::BLACK);

    return true;
  }
};

int main(int arc, char** argv) {
  Game game = Game();
  if (game.Construct(1920, 1080, 2, 2, true, false, true)) {
    game.Start();
  }
}