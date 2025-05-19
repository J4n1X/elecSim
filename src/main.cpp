#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "Grid.h"
#include "olcPixelGameEngine.h"

#define TILE_SCALE 1.0F

class Game : public olc::PixelGameEngine {
 public:
  Game() { sAppName = "Electricity Simulator"; }
  ~Game() {}

 private:
  // --- Constants ---
  static constexpr float defaultRenderScale = 15.0f;
  static const olc::vf2d defaultRenderOffset;

  // --- Layer indices ---
  int uiLayer = 0;
  int gameLayer = 0;

  // --- Simulation timing ---
  float accumulatedTime = 0.0f;
  float updateInterval = 0.2f;

  // --- Main grid ---
  Grid grid;

  // --- Game state ---
  bool paused = true;         // Simulation is paused, renderer is not
  bool engineRunning = true;  // If this is false, the game quits

  // --- Placement logic ---
  olc::vf2d lastPlacedPos = {0.0f, 0.0f};  // Prevents overwriting same tile
  std::unique_ptr<GridTile> brushTile;     // Next tile to be placed
  int selectedBrushIndex = 1;
  TileFacingSide selectedBrushFacing = TileFacingSide::Top;

  // --- Initialization ---
  bool OnUserCreate() override {
    accumulatedTime = 0.0f;
    updateInterval = 0.2f;
    uiLayer = 0;
    gameLayer = CreateLayer();
    EnableLayer((uint8_t)uiLayer, true);
    EnableLayer((uint8_t)gameLayer, true);
    grid = Grid(ScreenWidth(), ScreenHeight(), defaultRenderScale,
                defaultRenderOffset, uiLayer, gameLayer);
    paused = true;
    engineRunning = true;
    lastPlacedPos = olc::vf2d(0.0f, 0.0f);
    brushTile = nullptr;
    selectedBrushIndex = 1;
    selectedBrushFacing = TileFacingSide::Top;
    CreateBrushTile();  // Initialize brush tile; by default it's a wire
    return engineRunning;
  }

  // --- Brush tile creation ---
  void CreateBrushTile() {
    switch (selectedBrushIndex) {
      case 1:
        brushTile = std::make_unique<WireGridTile>();
        break;
      case 2:
        brushTile = std::make_unique<JunctionGridTile>();
        break;
      case 3:
        brushTile = std::make_unique<EmitterGridTile>();
        break;
      case 4:
        brushTile = std::make_unique<SemiConductorGridTile>();
        break;
      case 5:
        brushTile = std::make_unique<ButtonGridTile>();
        break;
      default:
        brushTile = nullptr;
        break;
    }
  }

  // --- Parse number key input (0-9) ---
  int ParseNumberFromInput() {
    for (int i = (int)olc::Key::K0; i <= (int)olc::Key::K9; i++) {
      olc::Key key = (olc::Key)i;
      if (GetKey(key).bPressed) {
        return i - (int)olc::Key::K0;
      }
    }
    return -1;  // No number key pressed
  }

  // --- User input processing (delegates to helpers) ---
  void ProcessUserInput(olc::vf2d& alignedWorldPos) {
    // 1. Mouse position and highlight
    auto selTileXIndex = GetMouseX();
    auto selTileYIndex = GetMouseY();
    auto hoverWorldPos =
        grid.ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
    alignedWorldPos = grid.AlignToGrid(hoverWorldPos);

    HandlePauseAndSpeed();
    if (!engineRunning) return;

    HandleCameraAndZoom(selTileXIndex, selTileYIndex, hoverWorldPos);

    HandleSaveLoad();

    HandleTileInteractions(alignedWorldPos, hoverWorldPos);
  }

  // --- Pause, speed, and quit controls ---
  void HandlePauseAndSpeed() {
    if (GetKey(olc::SPACE).bPressed) paused = !paused;
    if (GetKey(olc::PERIOD).bPressed) updateInterval += 0.05f;
    if (GetKey(olc::COMMA).bPressed)
      updateInterval = std::max(0.0f, updateInterval - 0.05f);
    if (GetKey(olc::ESCAPE).bPressed) engineRunning = false;
  }

  // --- Camera movement and zoom ---
  void HandleCameraAndZoom(int selTileXIndex, int selTileYIndex,
                           const olc::vf2d& hoverWorldPos) {
    auto renderScale = grid.GetRenderScale();
    auto curOffset = grid.GetRenderOffset();
    if (GetKey(olc::UP).bHeld)
      grid.SetRenderOffset(curOffset - olc::vf2d(0.0f, 0.25f));
    if (GetKey(olc::LEFT).bHeld)
      grid.SetRenderOffset(curOffset - olc::vf2d(0.25f, 0.0f));
    if (GetKey(olc::DOWN).bHeld)
      grid.SetRenderOffset(curOffset + olc::vf2d(0.0f, 0.25f));
    if (GetKey(olc::RIGHT).bHeld)
      grid.SetRenderOffset(curOffset + olc::vf2d(0.25f, 0.0f));
    if (GetKey(olc::J).bPressed || GetKey(olc::K).bPressed) {
      float newScale = renderScale + (GetKey(olc::J).bPressed ? 1 : -1);
      grid.SetRenderScale(newScale);
      auto afterZoomPos =
          grid.ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
      curOffset += (hoverWorldPos - afterZoomPos);
      grid.SetRenderOffset(curOffset);
    }
  }

  // --- Save/load controls ---
  void HandleSaveLoad() {
    if (GetKey(olc::S).bPressed) grid.Save("grid.bin");
    if (GetKey(olc::L).bPressed) grid.Load("grid.bin");
  }

  // --- Tile placement, removal, and interaction ---
  void HandleTileInteractions(const olc::vf2d& alignedWorldPos,
                              const olc::vf2d& hoverWorldPos) {
    // Right click: interact
    if (GetMouse(1).bPressed) {
      auto gridTileOpt = grid.GetTile(alignedWorldPos);
      if (gridTileOpt.has_value()) {
        auto& gridTile = gridTileOpt.value();
        auto updateFacing = gridTile->Interact();
        if (!updateFacing.IsEmpty())
          grid.QueueUpdate(alignedWorldPos, updateFacing);
      }
    }
    // Building mode controls
    if (paused) {
      // Select brush
      auto numInput = ParseNumberFromInput();
      if (numInput >= 0) {
        selectedBrushIndex = numInput;
        CreateBrushTile();
      }
      // Change facing
      if (GetKey(olc::R).bPressed) {
        selectedBrushFacing = TileUpdateFlags::RotateToFacing(
            selectedBrushFacing, TileFacingSide::Right);
      }
      // Place tile (left click)
      if (brushTile != nullptr) {
        if (GetMouse(0).bPressed ||
            (GetMouse(0).bHeld && lastPlacedPos != alignedWorldPos)) {
          brushTile->SetPos(alignedWorldPos);
          brushTile->SetFacing(selectedBrushFacing);
          std::shared_ptr<GridTile> sharedBrushTile = std::move(brushTile);
          grid.SetTile(alignedWorldPos, sharedBrushTile,
                       sharedBrushTile->IsEmitter());
          CreateBrushTile();
          lastPlacedPos = alignedWorldPos;
          grid.ResetSimulation();
        }
      }
      // Toggle default activation (right click)
      if (GetMouse(1).bPressed) {
        auto gridTileOpt = grid.GetTile(alignedWorldPos);
        if (gridTileOpt.has_value()) {
          auto& gridTile = gridTileOpt.value();
          gridTile->SetDefaultActivation(!gridTile->GetDefaultActivation());
          gridTile->ResetActivation();
        }
      }
      // Remove tile (middle click)
      if (GetMouse(2).bHeld) {
        grid.EraseTile(alignedWorldPos);
        grid.ResetSimulation();
      }
    }
  }

  // --- Main update loop ---
  bool OnUserUpdate(float fElapsedTime) override {
    // Handle window resizing
    auto curScreenSize = GetWindowSize() / GetPixelSize();
    if (grid.GetRenderWindow() != curScreenSize) {
      std::cout << "Window has been resized to " << curScreenSize << std::endl;
      grid.Resize(curScreenSize);
      SetScreenSize(curScreenSize.x, curScreenSize.y);
    }

    // Simulation step
    accumulatedTime += fElapsedTime;
    if (accumulatedTime > updateInterval && !paused) {
      grid.Simulate();
      accumulatedTime = 0;
    }

    // User input and UI
    olc::vf2d highlightWorldPos = {0.0f, 0.0f};
    ProcessUserInput(highlightWorldPos);

    // Status string
    std::stringstream ss;
    ss << '(' << highlightWorldPos.x << ", " << highlightWorldPos.y << ")"
       << " Selected: "
       << (brushTile != nullptr ? brushTile->TileTypeName() : "None") << "; "
       << "Facing: " << TileUpdateFlags::GetFacingName(selectedBrushFacing)
       << "; " << (paused ? "Paused" : "; Running") << '\n'
       << "Tiles: " << grid.GetTileCount() << "; "
       << "Updates: " << grid.GetUpdateCount() << '\n'
       << "Press ',' to increase and '.' to decrease speed";

    // Draw grid and UI
    grid.Draw(this, &highlightWorldPos);
    SetDrawTarget((uint8_t)(uiLayer & 0x0F));
    DrawString(olc::vi2d(0, 0), ss.str(), olc::BLACK);

    return engineRunning;
  }
};

// --- Main entry point ---
int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  Game game;
  if (game.Construct(640, 480, 2, 2, false, true, false)) {
    game.Start();
  }
}