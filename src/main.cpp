#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "Grid.h"
#include "olcPixelGameEngine.h"

#define TILE_SIZE 16

class Game : public olc::PixelGameEngine {
 public:
  Game() { sAppName = "Electricity Simulator"; }
  ~Game() {}

 private:
  const static float defaultTileSize;
  const static float defaultRenderScale;
  const static olc::vf2d defaultRenderOffset;

  int uiLayer;
  int gameLayer;

  float accumulatedTime;
  float updateInterval;

  Grid grid;

  // Game state variables
  bool paused;   // Simulation is paused, renderer is not
  bool running;  // If this is false, the game quits

  // Variables needed for the placement logic
  olc::vf2d lastPlacedPos;  // Position of last placed tile, to prevent
                            // overwriting the same tile over and over again
  std::unique_ptr<GridTile>
      brushTile;  // Variable which holds the next tile which should be placed
  int selectedBrushIndex;

  // std::shared_ptr<GridPalette> palette;

  bool OnUserCreate() override {
    accumulatedTime = 0.0f;
    updateInterval = 0.2f;

    // Create and enable the layers used for this project
    uiLayer = 0;
    gameLayer = CreateLayer();

    EnableLayer((uint8_t)uiLayer, true);
    EnableLayer((uint8_t)gameLayer, true);

    // Set world properties
    grid = Grid(ScreenWidth(), ScreenHeight(), defaultRenderScale,
                defaultRenderOffset, uiLayer, gameLayer);

    paused = false;
    running = true;

    lastPlacedPos = olc::vf2d(0.0f, 0.0f);
    brushTile = nullptr;
    selectedBrushIndex = 1;
    CreateBrushTile();  // Initialize brush tile; by default it's a wire

    return running;
  }

  void CreateBrushTile() {
    switch (selectedBrushIndex) {
      case 1:  //
        brushTile = std::make_unique<WireGridTile>();
        break;
      case 2:
        brushTile = std::make_unique<EmitterGridTile>();
        break;
    }
  }

  int ParseNumberFromInput() {
    for (int i = (int)olc::Key::K0; i <= (int)olc::Key::K9; i++) {
      olc::Key key = (olc::Key)i;

      if (GetKey(key).bPressed) {
        return i - (int)olc::Key::K0;
      }
    }
    // we didn't get any number input
    return -1;
  }

  void ProcessUserInput(olc::vf2d& alignedWorldPos) {
    // Highlight tile below mouse
    auto selTileXIndex = GetMouseX();
    auto selTileYIndex = GetMouseY();

    auto hoverWorldPos =
        grid.ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
    alignedWorldPos = grid.AlignToGrid(hoverWorldPos);

    // Game state manipulation
    if (GetKey(olc::SPACE).bPressed) {
      paused = !paused;
    }
    if (GetKey(olc::COMMA).bPressed) {  // Slow down simulation
      updateInterval += 0.05f;
    }
    if (GetKey(olc::PERIOD).bPressed) {  // Speed up simulation
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

    // Camera navigation
    auto renderScale = grid.GetRenderScale();
    auto curOffset = grid.GetRenderOffset();
    if (GetKey(olc::UP).bHeld) {
      grid.SetRenderOffset(curOffset - olc::vf2d(0.0f, 0.25f));
    }
    if (GetKey(olc::LEFT).bHeld) {
      grid.SetRenderOffset(curOffset - olc::vf2d(0.25f, 0.0f));
    }
    if (GetKey(olc::DOWN).bHeld) {
      grid.SetRenderOffset(curOffset + olc::vf2d(0.0f, 0.25f));
    }
    if (GetKey(olc::RIGHT).bHeld) {
      grid.SetRenderOffset(curOffset + olc::vf2d(0.25f, 0.0f));
    }

    if (GetKey(olc::K).bPressed) {  // Zoom In
      grid.SetRenderScale(renderScale + 1);
      // Relative to mouse will only work once world space works proper
      auto afterZoomPos =
          grid.ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
      curOffset += (hoverWorldPos - afterZoomPos);
      grid.SetRenderOffset(curOffset);
    } else if (GetKey(olc::L).bPressed) {  // Zoom out, else if to prevent both
                                           // in the same step
      grid.SetRenderScale(renderScale - 1);
      // Relative to mouse will only work once world space works proper
      auto afterZoomPos =
          grid.ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
      curOffset += (hoverWorldPos - afterZoomPos);
      grid.SetRenderOffset(curOffset);
    }

    // Building mode controls
    // TODO: Extract
    if (paused) {
      // Select brush tile if a number key was pressed
      auto numInput = ParseNumberFromInput();
      if (numInput >= 0) {
        selectedBrushIndex = numInput;
        CreateBrushTile();  // Switch to new tile type
      }

      // Modify tile if it's left-clicked
      if (brushTile != nullptr) {
        if (GetMouse(0).bPressed ||
            GetMouse(0).bHeld &&
                lastPlacedPos != alignedWorldPos) {  // Create wire
          // Now we modify the tile to suit our needs
          brushTile->SetPos(alignedWorldPos);
          brushTile->SetSize(Game::defaultTileSize);
#if 0  // Todo: Implement rotation
          brushTile->SetInputSides()
          brushTile->SetOutputSides()
#endif
          std::shared_ptr<GridTile> sharedBrushTile = std::move(brushTile);
          grid.SetTile(alignedWorldPos, sharedBrushTile,
                       sharedBrushTile->IsEmitter());
          CreateBrushTile();  // Create new brush tile to use for painting
          lastPlacedPos = alignedWorldPos;

          grid.ResetSimulation();
        }
      }
      if (GetMouse(1).bPressed) {  // Change default activation value
        auto gridTileOpt = grid.GetTile(alignedWorldPos);
        if (gridTileOpt.has_value()) {
          auto& gridTile = gridTileOpt.value();
          gridTile->SetDefaultActivation(!gridTile->DefaultActivation());
          gridTile->ResetActivation();
          grid.ResetSimulation();
        }
      }
      if (GetMouse(2).bHeld) {  // Remove tile
        grid.EraseTile(alignedWorldPos);
        grid.ResetSimulation();
      }
    }
  }

  bool OnUserUpdate(float fElapsedTime) override {
    // Check if the window has been resized
    auto curScreenSize = GetWindowSize() / GetPixelSize();
    if (grid.GetRenderWindow() != curScreenSize) {
      std::cout << "Window has been resized to " << curScreenSize << std::endl;
      grid.Resize(curScreenSize);
      SetScreenSize(curScreenSize.x, curScreenSize.y);
    }

    accumulatedTime += fElapsedTime;
    if (accumulatedTime > updateInterval && !paused) {
      grid.Simulate();
      accumulatedTime = 0;
    }
    olc::vf2d highlightWorldPos = olc::vf2d(0.0f, 0.0f);
    ProcessUserInput(highlightWorldPos);
    std::stringstream ss;
    ss << '(' << highlightWorldPos.x << ", " << highlightWorldPos.y << ')'
       << "Selected: "
       << (brushTile != nullptr ? brushTile->TileTypeName() : "None") << "; "
       << (paused ? "Paused" : "; Running") << '\n'
       << "Press '.' to increase and ',' to decrease speed";
    grid.Draw(this, &highlightWorldPos);
    SetDrawTarget((uint8_t)(uiLayer & 0x0F));
    DrawString(olc::vi2d(0, 0), ss.str(), olc::BLACK);

    return running;
  }
};
const olc::vf2d Game::defaultRenderOffset = olc::vf2d(0, 0);
const float Game::defaultTileSize = 1.0f;
const float Game::defaultRenderScale = 15.0f;

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  Game game = Game();
  if (game.Construct(640, 480, 2, 2, false, true, false)) {
    game.Start();
  }
}