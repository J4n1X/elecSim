#include <ranges>
#include <string>

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
  bool selectionActive = false;  // If true, selection is active (duh)
  int updatesPerTick = 0;

  // --- Highlight colors ---
  olc::Pixel highlightColor = olc::RED;

  // --- Prompt values ---
  std::string promptText = "";                            // Text for prompt
  std::function<void(std::string)> promptCall = nullptr;  // Callback for prompt

  // --- Placement logic ---
  olc::vi2d selectionStartIndex = {0, 0};  // Start tile index for selection
  olc::vi2d lastPlacedPos = {0, 0};        // Prevents overwriting same tile
  std::vector<std::unique_ptr<GridTile>>
      tileBuffer;  // Selected tiles for operations (stored as unique_ptr
                   // copies)
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
    selectedBrushIndex = 1;
    selectedBrushFacing = Direction::Top;
    CreateBrushTile();  // Initialize tile buffer with default brush tile
    return engineRunning;
  }

  // --- Create tile for placing in the buffer ---
  void CreateBrushTile() {
    // Clear the current buffer
    tileBuffer.clear();

    // Create a new tile based on the selected type
    std::unique_ptr<GridTile> newTile = nullptr;
    switch (selectedBrushIndex) {
      case 1:
        newTile = std::make_unique<WireGridTile>();
        std::cout << "Selected Wire tile" << std::endl;
        break;
      case 2:
        newTile = std::make_unique<JunctionGridTile>();
        std::cout << "Selected Junction tile" << std::endl;
        break;
      case 3:
        newTile = std::make_unique<EmitterGridTile>();
        std::cout << "Selected Emitter tile" << std::endl;
        break;
      case 4:
        newTile = std::make_unique<SemiConductorGridTile>();
        std::cout << "Selected Semiconductor tile" << std::endl;
        break;
      case 5:
        newTile = std::make_unique<ButtonGridTile>();
        std::cout << "Selected Button tile" << std::endl;
        break;
      case 6:
        newTile = std::make_unique<InverterGridTile>();
        std::cout << "Selected Inverter tile" << std::endl;
        break;
      case 7:
        newTile = std::make_unique<CrossingGridTile>();
        std::cout << "Selected Crossing tile" << std::endl;
        break;
      default:
        // Leave tileBuffer empty if no valid selection
        std::cout << "No tile selected" << std::endl;
        return;
    }

    // Set the facing direction
    newTile->SetFacing(selectedBrushFacing);

    // Add to buffer
    tileBuffer.push_back(std::move(newTile));
  }

  // | 0 1 0 |
  // | 1 0 1|
  // Should turn to
  // | 1 0 |
  // | 0 1 |
  // | 1 0 |
  // So the coords are
  // (1, 0) -> (1, 1)
  // (0, 1) -> (0, 0)
  // (2, 1) -> (0, 2)
  // But a rotation matrix does not support non-square matrices.
  // Thus, we expand into a square matrix, and then, in the end, justify.
  void RotateBufferTiles() {
    olc::vi2d minPos = {INT_MAX, INT_MAX};
    olc::vi2d maxPos = {INT_MIN, INT_MIN};

    // Rotate all tiles in the buffer to the next facing direction
    Direction newFacing =
        static_cast<Direction>((static_cast<int>(selectedBrushFacing) + 1) %
                               static_cast<int>(Direction::Count));

    if (selectedBrushFacing == newFacing) {
      throw std::runtime_error(
          "New facing should never be the same as the old facing");
    }
    selectedBrushFacing = newFacing;

    for (const auto& tile : tileBuffer) {
      olc::vi2d pos = tile->GetPos();
      maxPos.x = std::max(maxPos.x, pos.x);
      maxPos.y = std::max(maxPos.y, pos.y);
    }
    // Calculate width and height of the bounding box (must be square, so adjust
    // if needed)
    int boxSize = std::max(maxPos.x, maxPos.y);

    // Apply rotation to each tile
    for (auto& tile : tileBuffer) {
      olc::vi2d rotatedRelPos = {0, 0};
      int newX = 0, newY = 0;
      olc::vi2d relPos = tile->GetPos();
      newY = relPos.x;
      newX = boxSize - relPos.y;
      rotatedRelPos = olc::vi2d(newX, newY);
      rotatedRelPos.x = std::abs(rotatedRelPos.x);
      rotatedRelPos.y = std::abs(rotatedRelPos.y);
      minPos.x = std::min(minPos.x, rotatedRelPos.x);
      minPos.y = std::min(minPos.y, rotatedRelPos.y);
      tile->SetPos(rotatedRelPos);
    }

    // Justify - get the minimum
    olc::vi2d adjustVec = {0, 0};
    if (minPos.x > 0) {
      adjustVec.x = minPos.x;
    }
    if (minPos.y > 0) {
      adjustVec.y = minPos.y;
    }
    for (auto& tile : tileBuffer) {
      auto oldPos = tile->GetPos();
      auto oldFacing = tile->GetFacing();
      tile->SetPos(oldPos - adjustVec);
      tile->SetFacing(GridTile::RotateDirection(oldFacing, Direction::Right));
    }
    std::cout << "Rotated tile buffer to facing: "
              << static_cast<int>(selectedBrushFacing) << std::endl;
  }

  void ClearBuffer() {
    tileBuffer.clear();
    std::cout << "Cleared tile buffer" << std::endl;
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
  void ProcessUserInput(olc::vi2d& alignedWorldPos) {
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

    HandleTileInteractions(alignedWorldPos);

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

  // --- Tile selection and clipboard operations ---
  void CopyTiles(const olc::vi2d& startIndex, const olc::vi2d& endIndex) {
    // This sets the original facing direction to Top
    selectedBrushFacing = Direction::Top;

    // Get all tiles in the selection area as weak pointers
    auto weakSelection = grid.GetSelection(startIndex, endIndex);

    // Clear the current buffer
    tileBuffer.clear();

    // Create actual copies (clones) of the tiles and store them in our buffer
    for (auto& weakTile : weakSelection) {
      if (weakTile.expired()) continue;
      if (auto tilePtr = weakTile.lock()) {
        auto retTile = tilePtr->Clone();
        retTile->SetPos(retTile->GetPos() - startIndex);  // Adjust position
        tileBuffer.push_back(std::move(retTile));
      }
    }

    // Log selection information to console
    std::cout << "Copied tiles from selection: "
              << "Start: " << startIndex << ", End: " << endIndex
              << ", Count: " << tileBuffer.size() << std::endl;
  }

  void PasteTiles(const olc::vi2d& pastePosition) {
    // Check if we have something to paste
    if (tileBuffer.empty()) {
      return;  // Early return if buffer is empty
    }

    // Copy each tile in the selection
    for (const auto& tile : tileBuffer) {
      // Create a deep copy of the tile using Clone method
      std::unique_ptr<GridTile> newTile = tile->Clone();

      // Calculate new position relative to paste position
      auto newPos = pastePosition + tile->GetPos();
      newTile->SetPos(newPos);

      // Add the new tile to the grid
      bool isEmitter = newTile->IsEmitter();
      grid.SetTile(newPos, std::move(newTile), isEmitter);
    }

    grid.ResetSimulation();
    std::cout << "Pasted tiles at position: " << pastePosition << std::endl;
  }

  void CutTiles(const olc::vi2d& startIndex, const olc::vi2d& endIndex) {
    // First copy the tiles to buffer
    CopyTiles(startIndex, endIndex);

    // Then delete them from the grid
    // Ensure startIndex is actually the top-left corner
    olc::vi2d topLeft = startIndex.min(endIndex);
    olc::vi2d bottomRight = startIndex.max(endIndex);

    // Delete all tiles in the selection rectangle
    for (int y = topLeft.y; y <= bottomRight.y; y++) {
      for (int x = topLeft.x; x <= bottomRight.x; x++) {
        olc::vi2d pos(x, y);
        grid.EraseTile(pos);
      }
    }

    grid.ResetSimulation();
    std::cout << "Cut tiles from selection: "
              << "Start: " << startIndex << ", End: " << endIndex << std::endl;
  }
  // --- Tile placement, removal, and interaction ---
  void HandleTileInteractions(const olc::vi2d& alignedWorldPos) {
    // Building mode controls
    if (paused) {
      // Select brush
      auto numInput = ParseNumberFromInput();
      if (numInput >= 0) {
        selectedBrushIndex = numInput;
        CreateBrushTile();
      }

      // Start selection
      if (GetKey(olc::Key::CTRL).bPressed && !selectionActive) {
        // hoverWorldPos is already aligned to grid, so just downcast to vi2d
        selectionStartIndex = alignedWorldPos;
        selectionActive = true;
      }
      // Do something with the selection
      if (GetKey(olc::Key::CTRL).bHeld && selectionActive) {
        // Update the selection rectangle
        auto endTileIndex = alignedWorldPos;
        // Copying
        if (GetKey(olc::Key::C).bPressed) {
          CopyTiles(selectionStartIndex, endTileIndex);
        }
        // Pasting
        if (GetKey(olc::Key::V).bPressed) {
          PasteTiles(selectionStartIndex);
        }
        // Cutting
        if (GetKey(olc::Key::X).bPressed) {
          CutTiles(selectionStartIndex, endTileIndex);
        }
      }

      // End selection
      if (GetKey(olc::Key::CTRL).bReleased && selectionActive) {
        selectionActive = false;
      }

      // Change facing for all tiles in buffer
      if (GetKey(olc::Key::R).bPressed) {
        RotateBufferTiles();
      }

      // Clear buffer with P (when not exiting the app)
      if (GetKey(olc::Key::P).bPressed) {
        ClearBuffer();
      }
      // Place tiles from buffer (left click)
      if (!tileBuffer.empty()) {
        if (GetMouse(0).bPressed ||
            (GetMouse(0).bHeld && lastPlacedPos != alignedWorldPos)) {
          // Paste buffer
          PasteTiles(alignedWorldPos);

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
    olc::vi2d highlightWorldPos = {0, 0};
    ProcessUserInput(highlightWorldPos);

    if (paused) updatesPerTick = 0;

    // Draw grid and UI
    int drawnTiles = grid.Draw(this);

    if (selectionActive) {
      olc::vi2d start = selectionStartIndex;
      olc::vi2d end = highlightWorldPos;
      // Ensure start is the top-left and end is the bottom-right
      // We also need to encount for entering the negative domain
      olc::vi2d topLeft = start.min(end);
      olc::vi2d bottomRight = start.max(end);

      olc::vi2d gridSize = bottomRight - topLeft + olc::vi2d(1, 1);
      if (gridSize.x < 0 || gridSize.y < 0) {
        throw std::runtime_error(
            "Selection rectangle has negative size, this should not happen");
      }
      olc::vf2d posScreen = grid.WorldToScreenFloating(topLeft);
      olc::vf2d sizeScreen = {gridSize.x * grid.GetRenderScale(),
                              gridSize.y * grid.GetRenderScale()};
      SetDrawTarget(uiLayer);
      DrawRectDecal(posScreen, sizeScreen, highlightColor);
    } else {
      SetDrawTarget(uiLayer);
      DrawRectDecal(grid.WorldToScreenFloating(highlightWorldPos),
                    {grid.GetRenderScale(), grid.GetRenderScale()},
                    highlightColor);
    }

    // Draw the current buffer tiles half transparent (preview)
    // TODO: We should move the scale variables out of the Grid, it has no right
    // to manage those
    // TODO: Actually finish implementing the bounding box highlight.

    if (!tileBuffer.empty() && !selectionActive) {
      // First, show a light rectangular highlight around the entire buffer area
      // if multiple tiles

      // if (tileBuffer.size() > 1) {
      //   // Calculate bounding box of all tiles in buffer
      //   olc::vi2d minPos = {INT_MAX, INT_MAX};
      //   olc::vi2d maxPos = {INT_MIN, INT_MIN};

      //   for (const auto& tile : tileBuffer) {
      //     olc::vi2d relPos = tile->GetPos() + highlightWorldPos;
      //     minPos.x = std::min(minPos.x, relPos.x);
      //     minPos.y = std::min(minPos.y, relPos.y);
      //     maxPos.x = std::max(maxPos.x, relPos.x);
      //     maxPos.y = std::max(maxPos.y, relPos.y);
      //   }

      //   // Draw highlight for the bounding box
      //   olc::vi2d size = maxPos - minPos + olc::vi2d(1, 1);
      //   olc::vf2d sizeScreen = {size.x * grid.GetRenderScale(),
      //                           size.y * grid.GetRenderScale()};

      //   olc::vf2d topLeftScreen =
      //       grid.WorldToScreenFloating(highlightWorldPos + minPos);
      // }

      // Then draw each tile with transparency
      for (const auto& tile : tileBuffer) {
        // Calculate position for preview
        auto previewPos = tile->GetPos() + highlightWorldPos;

        // Draw with transparency
        tile->Draw(this, grid.WorldToScreen(previewPos), grid.GetRenderScale(),
                   128);
      }
    }

    // Status string
    std::stringstream ss;
    ss << '(' << highlightWorldPos.x << ", " << highlightWorldPos.y << ")"
       << " Buffer: ";

    if (tileBuffer.empty()) {
      ss << "Empty";
    } else if (tileBuffer.size() == 1) {
      ss << tileBuffer[0]->TileTypeName().data();
    } else {
      ss << tileBuffer.size() << " tiles";

      // Count tile types
      std::map<std::string, int> typeCounts;
      for (const auto& tile : tileBuffer) {
        typeCounts[std::string(tile->TileTypeName())]++;
      }

      ss << " (";
      bool first = true;
      for (const auto& [type, count] : typeCounts) {
        if (!first) ss << ", ";
        ss << count << " " << type;
        first = false;
      }
      ss << ")";
    }

    ss << "; "
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
      ConsoleOut() << "Unknown command: " << command << std::endl;
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
  if (argv[1] != nullptr) {
    std::string arg = argv[1];
    if (arg == "dry") {
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