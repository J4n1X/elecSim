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
  bool paused;
  bool running;

  int uiLayer;
  int gameLayer;

  float accumulatedTime;
  float updateInterval;

  Grid grid;
  // std::shared_ptr<GridPalette> palette;

  bool OnUserCreate() override {
    accumulatedTime = 0.0f;
    updateInterval = 0.2f;

    // Create and enable the layers used for this project
    uiLayer = 0;
    gameLayer = CreateLayer();

    EnableLayer(uiLayer, true);
    EnableLayer(gameLayer, true);

    // Set global world properties
    Grid::worldScale = 15;
    Grid::worldOffset = olc::vi2d(0, 0);

    grid = Grid(ScreenWidth(), ScreenHeight(), uiLayer, gameLayer);
    // palette = grid.GetPalette();

    running = true;
    paused = false;

    return running;
  }

  int ParseNumberFromInput() {
    int parsedNumber = 0;

    for (int i = static_cast<int>(olc::Key::K0);
         i <= static_cast<int>(olc::Key::K9); i++) {
      olc::Key key = static_cast<olc::Key>(i);

      if (GetKey(key).bPressed) {
        return i - static_cast<int>(olc::Key::K0);
      }
    }
    // we didn't get any
    return -1;
  }

  void ProcessUserInput(olc::vi2d& alignedWorldPos) {
    // Highlight tile below mouse
    auto selTileXIndex = GetMouseX();
    auto selTileYIndex = GetMouseY();

    alignedWorldPos =
        Grid::ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
    auto index = olc::vi2d((int)alignedWorldPos.x, (int)alignedWorldPos.y);

    // Game state manipulation
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
      running = false;
      return;
    }

    // Building mode controls
    if (paused) {
      // Building Tile Selector {
      auto numInput = ParseNumberFromInput();
      if (numInput >= 0) {  // We did get a numeric input
        // if (numInput <= palette->GetBrushCount()) {
        // }
      }

      // Modify tile if it's left-clicked
      auto tile = grid.GetTile(olc::vi2d());
      auto size = tile != nullptr ? tile->GetSize() : 1;
      if (GetMouse(0).bHeld) {  // Create wire
        auto wire = std::make_shared<WireGridTile>(alignedWorldPos, size);
        grid.SetTile(index, wire, false);
      }
      if (GetMouse(1).bPressed) {  // Change default activation value
        auto tile = grid.GetTile(index);
        if (tile != nullptr) {
          tile->SetDefaultActivation(!tile->DefaultActivation());
        }
      }
      if (GetMouse(2).bHeld) {  // Remove tile
        grid.EraseTile(index);
      }
    }
  }

  bool OnUserUpdate(float fElapsedTime) override {
    accumulatedTime += fElapsedTime;
    if (accumulatedTime > updateInterval && !paused) {
      grid.Simulate();
      accumulatedTime = 0;
    }
    olc::vi2d highlightWorldPos = olc::vi2d(0, 0);
    ProcessUserInput(highlightWorldPos);

    // std::stringstream ss;
    // ss << "Selected: " << palette->GetBrushName() << "; "
    //    << (paused ? "Paused" : "; Running") << '\n'
    //    << "Press '.' to increase and ',' to decrease speed";
    // SetDrawTarget(uiLayer);
    // DrawString(olc::vi2d(0, 0), ss.str(), olc::BLACK);

    grid.Draw(this, &highlightWorldPos);

    return running;
  }
};

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  Game game = Game();
  if (game.Construct(640, 480, 2, 2, false, false, true)) {
    game.Start();
  }
}