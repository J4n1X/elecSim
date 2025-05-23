#include <cstddef>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include "Grid.h"
#include "GridTileTypes.h"
#include "nfd.hpp"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

using namespace ElecSim;

class Game : public olc::PixelGameEngine {
 public:
  Game() {
    sAppName = "Electricity Simulator";
    // Print class sizes on startup
  }
  ~Game() {}

  // Print memory sizes of all tile classes (for debugging)
  void PrintClassSizes() {
    ConsoleOut() << "\n=== Class Memory Sizes ===\n";
    ConsoleOut() << "Base GridTile: " << sizeof(GridTile) << " bytes\n";
    ConsoleOut() << "WireGridTile: " << sizeof(WireGridTile) << " bytes\n";
    ConsoleOut() << "JunctionGridTile: " << sizeof(JunctionGridTile)
                 << " bytes\n";
    ConsoleOut() << "EmitterGridTile: " << sizeof(EmitterGridTile)
                 << " bytes\n";
    ConsoleOut() << "SemiConductorGridTile: " << sizeof(SemiConductorGridTile)
                 << " bytes\n";
    ConsoleOut() << "ButtonGridTile: " << sizeof(ButtonGridTile) << " bytes\n";
    ConsoleOut() << "SignalEvent: " << sizeof(SignalEvent) << " bytes\n";
    ConsoleOut() << "UpdateEvent: " << sizeof(UpdateEvent) << " bytes\n";
    ConsoleOut() << "============================\n\n";
  }

 private:
  // --- Constants ---
  static constexpr float defaultRenderScale = 32.0f;
  static const olc::vf2d defaultRenderOffset;
  static constexpr float minRenderScale = 2.0f;
  static constexpr float maxRenderScale = 256.0f;

  // --- Layer indices ---
  int uiLayer = 0;
  int gameLayer = 0;

  // --- Simulation timing ---
  float accumulatedTime = 0.0f;
  float updateInterval = 0.1f;

  // --- Main grid ---
  Grid grid;

  // --- Game state ---
  bool paused = true;         // Simulation is paused, renderer is not
  bool engineRunning = true;  // If this is false, the game quits
  bool consoleLogging =
      false;  // If this is true, stdout is redirected to the console
  int updatesPerTick = 0;

  // --- Prompt values ---
  std::string promptText = "";                            // Text for prompt
  std::function<void(std::string)> promptCall = nullptr;  // Callback for prompt

  // --- Placement logic ---
  olc::vf2d lastPlacedPos = {0.0f, 0.0f};  // Prevents overwriting same tile
  std::unique_ptr<GridTile> brushTile;     // Next tile to be placed
  int selectedBrushIndex = 1;
  Direction selectedBrushFacing = Direction::Top;

  // --- Initialization ---
  bool OnUserCreate() override {
    // Print class sizes
    PrintClassSizes();

    gameLayer = CreateLayer();  // initialized as a black screen

    SetDrawTarget(gameLayer);
    Clear(olc::BLUE);
    SetDrawTarget(uiLayer);
    Clear(olc::BLANK);  // uiLayer needs to be transparent

    EnableLayer((uint8_t)uiLayer, true);
    EnableLayer((uint8_t)gameLayer, true);

    grid = Grid(ScreenWidth(), ScreenHeight(), defaultRenderScale,
                defaultRenderOffset, uiLayer, gameLayer);
    paused = true;
    engineRunning = true;
    lastPlacedPos = olc::vf2d(0.0f, 0.0f);
    brushTile = nullptr;
    selectedBrushIndex = 1;
    selectedBrushFacing = Direction::Top;
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
      case 6:
        brushTile = std::make_unique<InverterGridTile>();
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
    // If we're in text entry mode, we can't process this.
    if (IsTextEntryEnabled()) return;

    // 1. Mouse position and highlight
    auto selTileXIndex = GetMouseX();
    auto selTileYIndex = GetMouseY();
    auto hoverWorldPos =
        grid.ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
    alignedWorldPos = grid.AlignToGrid(hoverWorldPos);

    if (!engineRunning) return;
    HandlePauseAndSpeed();

    HandleCameraAndZoom(selTileXIndex, selTileYIndex, hoverWorldPos);

    HandleTileInteractions(alignedWorldPos, hoverWorldPos);

    HandleSaveLoad();

    // When we want to open the console, the key is F1
    if (GetKey(olc::Key::F1).bPressed) {
      ConsoleShow(olc::Key::F1, false);
    }
  }

  // --- Pause, speed, and quit controls ---
  void HandlePauseAndSpeed() {
    if (GetKey(olc::Key::SPACE).bPressed) paused = !paused;
    if (GetKey(olc::Key::PERIOD).bPressed) updateInterval += 0.025f;
    if (GetKey(olc::Key::COMMA).bPressed)
      updateInterval = std::max(0.0f, updateInterval - 0.025f);
    if (GetKey(olc::Key::ESCAPE).bPressed) engineRunning = false;
  }

  // --- Camera movement and zoom ---
  void HandleCameraAndZoom(int selTileXIndex, int selTileYIndex,
                           const olc::vf2d& hoverWorldPos) {
    auto renderScale = grid.GetRenderScale();
    auto curOffset = grid.GetRenderOffset();
    if (GetKey(olc::Key::UP).bHeld)
      grid.SetRenderOffset(curOffset + olc::vf2d(0.0f, 0.25f * renderScale));
    if (GetKey(olc::Key::LEFT).bHeld)
      grid.SetRenderOffset(curOffset + olc::vf2d(0.25f * renderScale, 0.0f));
    if (GetKey(olc::Key::DOWN).bHeld)
      grid.SetRenderOffset(curOffset - olc::vf2d(0.0f, 0.25f * renderScale));
    if (GetKey(olc::Key::RIGHT).bHeld)
      grid.SetRenderOffset(curOffset - olc::vf2d(0.25f * renderScale, 0.0f));
    if (GetKey(olc::Key::J).bHeld || GetKey(olc::Key::K).bHeld) {
      float newScale = std::clamp(
          renderScale +
              ((GetKey(olc::Key::J).bHeld ? 1 : -1) * renderScale * 0.1f),
          minRenderScale, maxRenderScale);
      grid.SetRenderScale(newScale);
      auto afterZoomPos =
          grid.ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
      // So, now we have the before and after in world space.
      // We want to keep the same world position in screen space.
      auto newOffset = curOffset + (afterZoomPos - hoverWorldPos) * newScale;
      grid.SetRenderOffset(newOffset);
    }
  }

  // --- Save/load controls ---
  void HandleSaveLoad() {
    constexpr nfdfilteritem_t filterItem[] = {
        {"Grid files", "grid"},
    };
    NFD::Init();
    NFD::UniquePath resultPath;
    std::string curPath = std::filesystem::current_path().string();

    auto checkResultAndYield = [&](nfdresult_t result) {
      std::string filename = "";
      if (result == NFD_OKAY) {
        filename = resultPath.get();
      } else if (result == NFD_CANCEL) {
        std::cout << "User cancelled the load dialog." << std::endl;
      } else {
        std::cerr << "Error: " << NFD::GetError() << std::endl;
      }
      return filename;
    };
    if (GetKey(olc::Key::F2).bPressed) {
      auto result = NFD::SaveDialog(resultPath, filterItem, 1, curPath.c_str());
      std::string filename = checkResultAndYield(result);
      if (filename != "") {
        grid.Save(filename);
      }
    }
    if (GetKey(olc::Key::F3).bPressed) {
      auto result = NFD::OpenDialog(resultPath, filterItem, 1, curPath.c_str());
      std::string filename = checkResultAndYield(result);
      if (filename != "") {
        grid.Load(filename);
      }
    }
    NFD::Quit();
  }

  // --- Tile placement, removal, and interaction ---
  void HandleTileInteractions(const olc::vf2d& alignedWorldPos,
                              const olc::vf2d& hoverWorldPos) {
    // Building mode controls
    if (paused) {
      // Select brush
      auto numInput = ParseNumberFromInput();
      if (numInput >= 0) {
        selectedBrushIndex = numInput;
        CreateBrushTile();
      }
      // Change facing
      if (GetKey(olc::Key::R).bPressed) {
        selectedBrushFacing =
            static_cast<Direction>((static_cast<int>(selectedBrushFacing) + 1) %
                                   static_cast<int>(Direction::Count));
      }
      // Place tile (left click)
      if (brushTile != nullptr) {
        if (GetMouse(0).bPressed ||
            (GetMouse(0).bHeld && lastPlacedPos != alignedWorldPos)) {
          brushTile->SetPos(alignedWorldPos);
          brushTile->SetFacing(selectedBrushFacing);
          bool isEmitter = brushTile->IsEmitter();
          grid.SetTile(alignedWorldPos, std::move(brushTile), isEmitter);
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
    } else {
      // Right click: interact
      if (GetMouse(1).bPressed) {
        auto gridTileOpt = grid.GetTile(alignedWorldPos);
        if (gridTileOpt.has_value()) {
          auto& gridTile = gridTileOpt.value();
          auto signals = gridTile->Interact();
          for (const auto& signal : signals) {
            grid.QueueUpdate(gridTile, signal);
          }
        }
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
      updatesPerTick = grid.Simulate();
      accumulatedTime = 0;
    }

    // User input
    olc::vf2d highlightWorldPos = {0.0f, 0.0f};
    ProcessUserInput(highlightWorldPos);

    if (paused) updatesPerTick = 0;

    // Draw grid and UI
    int drawnTiles = grid.Draw(this, &highlightWorldPos);
    if (!IsTextEntryEnabled()) {
      // Status string
      std::stringstream ss;
      ss << '(' << highlightWorldPos.x << ", " << highlightWorldPos.y << ")"
         << " Selected: "
         << (brushTile != nullptr ? brushTile->TileTypeName() : "None") << "; "
         << "Facing: "
         << (selectedBrushFacing == Direction::Top      ? "Top"
             : selectedBrushFacing == Direction::Right  ? "Right"
             : selectedBrushFacing == Direction::Bottom ? "Bottom"
                                                        : "Left")
         << "; " << (paused ? "Paused" : "; Running") << '\n'
         << "Tiles: " << grid.GetTileCount() << " (" << drawnTiles << " visible"
         << ")" << "; "
         << "Updates: " << updatesPerTick << " per tick" << '\n'
         << "Press ',' to increase and '.' to decrease speed";

      SetDrawTarget((uint8_t)(uiLayer & 0x0F));
      Clear(olc::BLANK);
      DrawString(olc::vi2d(0, 0), ss.str(), olc::BLACK, 2);
    } else {
      // Freeze the program and show prompt
      paused = true;
      auto outputText = promptText + " " + TextEntryGetString();
      SetDrawTarget((uint8_t)(uiLayer & 0x0F));
      Clear(olc::BLANK);
      DrawString(olc::vi2d(0, 0), outputText, olc::BLACK, 2);
    }

    return engineRunning;
  }

  void OnTextEntryComplete(const std::string& text) override {
    std::cout << "Text entry complete: " << text << std::endl;
    if (promptCall) {
      promptCall(text);
    }
  }

  bool OnConsoleCommand(const std::string& command) override {
    if (command == "exit") {
      engineRunning = false;
    } else if (command == "toggleconsole") {
      consoleLogging = !consoleLogging;
      ConsoleOut() << "Console logging "
                   << (consoleLogging ? "enabled" : "disabled") << std::endl;
      ConsoleCaptureStdOut(consoleLogging);
    } else if (command == "pause") {
      paused = !paused;
      ConsoleOut() << "Simulation " << (paused ? "paused" : "running")
                   << std::endl;
    } else if (command == "reset") {
      grid.ResetSimulation();
      std::cout << "Simulation reset" << std::endl;
    } else if (command == "clear") {
      ConsoleClear();
    } else if (command == "new") {
      grid.Clear();
      ConsoleOut() << "Grid cleared" << std::endl;
    } else if (command == "help") {
      ConsoleOut() << "Available commands: exit, pause, reset, clear, new, help"
                   << std::endl;
    } else {
      ConsoleOut()  << "Unknown command: " << command << std::endl;
    }
    return true;
  }

  bool OnUserDestroy() override {
    ConsoleCaptureStdOut(false);
    ConsoleOut() << std::endl;
    return true;
  }
};

// --- Static member initialization ---
const olc::vf2d Game::defaultRenderOffset = olc::vf2d(0, 0);

// --- Main entry point ---
int main(int argc, char** argv) {
  (void)argc;
  if(argv[1] != nullptr){
    std::string arg = argv[1];
    if(arg == "dry"){
      std::cout << "Dry run mode enabled. No GUI will be created." << std::endl;
      return 0;
    }
  }

  Game game;
  if (game.Construct(640 * 2, 480 * 2, 1, 1, false, true, false)) {
    game.Start();
  }
  return 0;
}