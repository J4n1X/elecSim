#include "ControlManager.h"
#include "Grid.h"
#include "GridTileTypes.h"
#include "MessageBox.h"
#include "nfd.hpp"
#include "olcPixelGameEngine.h"

#include <ranges>

using namespace ElecSim;

class Game : public olc::PixelGameEngine {
 public:
  Game()
      : controlManager(this),
        unsavedChangesGui(this,
                          "Unsaved changes detected. Do you want to save?",
                          Game::defaultRenderScale * 5.f, false) {
    sAppName = std::format(appNameBaseFmt, "New File");
  }
  ~Game() {}
  bool SetStartFile(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) {
      curFilename = path;
      return true;
    }
    return false;
  }

 private:
  // --- Constants ---
  static constexpr float defaultRenderScale = 32.0f;
  static const olc::vf2d defaultRenderOffset;
  static constexpr float minRenderScale = 12.0f;
  static constexpr float maxRenderScale = 256.0f;
  static constexpr const char* appNameBaseFmt =
      "Electricity Simulator - {}";

  // --- Layer indices ---
  int uiLayer = 0;
  int gameLayer = 0;

  // --- Simulation timing ---
  float accumulatedTime = 0.0f;
  float updateInterval = 0.1f;

  // --- Main grid ---
  Engine::ControlManager controlManager;
  Grid grid;
  MessageBoxGui unsavedChangesGui;

  // --- Game state ---
  bool paused = true;          // Simulation is paused, renderer is not
  bool engineRunning = false;  // If this is false, the game quits
  bool consoleLogging =
      false;  // If this is true, stdout is redirected to the console
  bool selectionActive = false;
  bool unsavedChanges = false;
  int updatesPerTick = 0;
  std::filesystem::path curFilename = "";

  // --- Highlight colors ---
  olc::Pixel highlightColor = olc::RED;

  // --- Placement logic ---
  olc::vi2d selectionStartIndex = {0, 0};  // Start tile index for selection
  olc::vi2d lastPlacedPos = {0, 0};        // Prevents overwriting same tile
  std::vector<std::unique_ptr<GridTile>>
      tileBuffer;  // Selected tiles for operations (stored as unique_ptr
                   // copies)
  olc::vi2d tileBufferBoxSize = {
      0, 0};  // Size of the tile buffer box (calculated when copies are made)
  int selectedBrushIndex = 1;
  Direction selectedBrushFacing = Direction::Top;

  // --- Initialization ---
  bool OnUserCreate() override {
#ifndef DEBUG 
    // Capturing stdout to console (Release only)
    ConsoleCaptureStdOut(true);
#endif

    gameLayer = CreateLayer();  // initialized as a black screen

    SetDrawTarget(gameLayer);
    Clear(olc::BLUE);
    SetDrawTarget(uiLayer);
    Clear(olc::BLANK);  // uiLayer needs to be transparent

    EnableLayer((uint8_t)uiLayer, true);
    EnableLayer((uint8_t)gameLayer, true);

    grid = Grid(ScreenWidth(), ScreenHeight(), defaultRenderScale,
                defaultRenderOffset, gameLayer);

    if (curFilename.empty()) {
      curFilename =
          std::filesystem::current_path().append("default.grid").string();
    } else {
      grid.Load(curFilename.string());
      sAppName = std::format(appNameBaseFmt, curFilename.filename().string());
    }

    paused = true;
    engineRunning = true;
    CreateBrushTile();  // Initialize tile buffer with default brush tile
    return engineRunning;
  }

  // --- Create tile for placing in the buffer ---
  void CreateBrushTile() {
    // Clear the current buffer
    tileBuffer.clear();

    auto createTile = [this]() -> std::unique_ptr<GridTile> {
      switch (selectedBrushIndex) {
        case 1:
          return std::make_unique<WireGridTile>();
        case 2:
          return std::make_unique<JunctionGridTile>();
        case 3:
          return std::make_unique<EmitterGridTile>();
        case 4:
          return std::make_unique<SemiConductorGridTile>();
        case 5:
          return std::make_unique<ButtonGridTile>();
        case 6:
          return std::make_unique<InverterGridTile>();
        case 7:
          return std::make_unique<CrossingGridTile>();
        default:
          return nullptr;
      }
    };

    auto newTile = createTile();
    if (!newTile) {
      return;  // No valid selection
    }

    // Set the facing direction
    newTile->SetFacing(selectedBrushFacing);
    tileBufferBoxSize = {1, 1};  // Default size for single tile

    // Add to buffer
    tileBuffer.push_back(std::move(newTile));
  }

  void CalculateTileBufferBoxSize() {
    if (tileBuffer.empty()) {
      // This hides the highlight when no tiles are in the buffer
      tileBufferBoxSize = {0, 0};
      return;
    }

    olc::vi2d minPos = {INT_MAX, INT_MAX};
    olc::vi2d maxPos = {INT_MIN, INT_MIN};

    for (const auto& tile : tileBuffer) {
      olc::vi2d pos = tile->GetPos();
      minPos.x = std::min(minPos.x, pos.x);
      minPos.y = std::min(minPos.y, pos.y);
      maxPos.x = std::max(maxPos.x, pos.x);
      maxPos.y = std::max(maxPos.y, pos.y);
    }

    // Calculate the size of the bounding box
    tileBufferBoxSize.x =
        maxPos.x - minPos.x + 1;  // +1 to include the last tile
    tileBufferBoxSize.y =
        maxPos.y - minPos.y + 1;  // +1 to include the last tile
  }

  void JustifyBufferTiles() {
    olc::vi2d minPos = std::ranges::fold_right(
        tileBuffer, olc::vi2d{INT_MAX, INT_MAX},
        [](const auto& tile, const olc::vi2d& acc) {
          return olc::vi2d{std::min(acc.x, tile->GetPos().x),
                           std::min(acc.y, tile->GetPos().y)};
        });
    for (auto& tile : tileBuffer) {
      auto clampedMinPos =
          olc::vi2d{std::max(0, minPos.x), std::max(0, minPos.y)};
      auto newPos = tile->GetPos() - clampedMinPos;
      tile->SetPos(newPos);
    }
  }

  // | 0 1 0 |
  // | 1 0 1|
  // Should turn to
  // | 1 0 |
  // | 0 1 |
  // | 1 0 |
  // So the coords are  // (1, 0) -> (1, 1)
  // (0, 1) -> (0, 0)
  // (2, 1) -> (0, 2)
  // But a rotation matrix does not support non-square matrices.
  // Thus, we expand into a square matrix, and then, in the end, justify.
  void RotateBufferTiles() {
    olc::vi2d maxPos = {INT_MIN, INT_MIN};

    // Rotate all tiles in the buffer to the next facing direction
    Direction newFacing =
        DirectionRotate(selectedBrushFacing, Direction::Right);

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
    // Calculate the far offset (must be the same in X and Y, so adjust
    // if needed)
    int farOffset = std::max(maxPos.x, maxPos.y);

    // Apply rotation to each tile
    for (auto& tile : tileBuffer) {
      olc::vi2d rotatedRelPos = {0, 0};
      int newX = 0, newY = 0;
      olc::vi2d relPos = tile->GetPos();
      newY = relPos.x;
      newX = farOffset - relPos.y;
      rotatedRelPos.x = std::abs(newX);
      rotatedRelPos.y = std::abs(newY);
      tile->SetPos(rotatedRelPos);
      tile->SetFacing(DirectionRotate(tile->GetFacing(), Direction::Right));
    }

    JustifyBufferTiles();
    CalculateTileBufferBoxSize();
  }

  void ClearBuffer() {
    tileBuffer.clear();
    CalculateTileBufferBoxSize();
    // std::cout << "Cleared tile buffer" << std::endl;
  }

  void Reset() { grid.ResetSimulation(); }

  // --- Event handling method for ControlManager ---
  void HandleGameEvent(Engine::GameStates::Event event, float deltaTime,
                       olc::vi2d& highlightWorldPos) {
    using enum Engine::GameStates::Event;
    switch (event) {
      // General events
      case ConsoleToggle:
        ConsoleShow(olc::Key::F1, false);
        break;

      case Save:
        SaveGrid();
        break;

      case Load:
        if (unsavedChanges) {
          unsavedChangesGui.Enable();
        } else {
          LoadGrid();
        }
        break;

      case Quit:
        engineRunning = false;
        break;

      case BuildModeToggle:
        // Reset Simulation to provide consistent behavior
        Reset();
        paused = !paused;
        break;

      // Camera events
      case CameraMoveUp:
        HandleCameraMove(0.0f, 30.f * grid.GetRenderScale() * deltaTime);
        break;

      case CameraMoveDown:
        HandleCameraMove(0.0f, -30.f * grid.GetRenderScale() * deltaTime);
        break;

      case CameraMoveLeft:
        HandleCameraMove(30.f * grid.GetRenderScale() * deltaTime, 0.0f);
        break;

      case CameraMoveRight:
        HandleCameraMove(-30.f * grid.GetRenderScale() * deltaTime, 0.0f);
        break;

      case CameraZoomIn:
      case CameraZoomOut:
        HandleCameraZoom(event == CameraZoomIn ? 1 : -1);
        break;

      case CameraReset:
        grid.SetRenderOffset(defaultRenderOffset);
        grid.SetRenderScale(defaultRenderScale);
        break;

      case CameraPan: {
        auto panDelta =
            controlManager.GetMousePosition() - controlManager.GetPanStartPos();
        grid.SetRenderOffset(
            (grid.GetRenderOffset() - panDelta / grid.GetRenderScale()));
        break;
      }

      // Build mode events
      case BuildModeTileSelect:
        if (paused) {
          selectedBrushIndex = controlManager.GetLastSelectedNumberKey();
          CreateBrushTile();
        }
        break;

      case BuildModeRotate:
        if (paused) {
          RotateBufferTiles();
        }
        break;

      case BuildModeCopy:
        if (paused && selectionActive) {
          CopyTiles(selectionStartIndex, highlightWorldPos);
        }
        break;

      case BuildModePaste:
        if (paused) {
          PasteTiles(highlightWorldPos);
        }
        break;
      case BuildModeCut:
        if (paused) {
          // Handle both cut selection and clear buffer
          CutTiles(selectionStartIndex, highlightWorldPos);
        }
        break;
      case BuildModeDelete:
        if (paused) {
          // Delete tiles in the selection area
          grid.EraseTile(highlightWorldPos);
          selectionActive = false;  // Clear selection after deletion
        }
        break;

      case BuildModeSelect:
        if (paused) {
          // Toggle selection state - this is now a discrete event
          if (!selectionActive) {
            selectionStartIndex = highlightWorldPos;
            selectionActive = true;
          } else {
            selectionActive = false;
          }
        }
        break;
      case BuildModeClearBuffer:
        if (paused) {
          ClearBuffer();
        }
        break;

      // Simulation mode events
      case SimModeTileInteract:
        if (!paused) {
          auto gridTileOpt = grid.GetTile(highlightWorldPos);
          if (gridTileOpt.has_value()) {
            auto& gridTile = gridTileOpt.value();
            auto signals = gridTile->Interact();
            for (const auto& signal : signals) {
              grid.QueueUpdate(gridTile, signal);
            }
          }
        }
        break;

      case SimModeTickSpeedUp:
        updateInterval += 0.025f;
        break;

      case SimModeTickSpeedDown:
        updateInterval = std::max(0.0f, updateInterval - 0.025f);
        break;
    }
  }

  // --- Helper methods for camera movement ---
  void HandleCameraMove(float deltaX, float deltaY) {
    grid.SetRenderOffset(grid.GetRenderOffset() + olc::vf2d(deltaX, deltaY));
  }

  void HandleCameraZoom(int direction) {
    olc::vf2d hoverWorldPos = grid.ScreenToWorld(
        static_cast<olc::vi2d>(controlManager.GetMousePosition()));
    float renderScale = grid.GetRenderScale();

    float newScale = std::clamp(renderScale + (direction * renderScale * 0.1f),
                                minRenderScale, maxRenderScale);
    grid.SetRenderScale(newScale);

    olc::vf2d afterZoomPos = grid.ScreenToWorld(
        static_cast<olc::vi2d>(controlManager.GetMousePosition()));
    olc::vf2d newOffset =
        grid.GetRenderOffset() + (afterZoomPos - hoverWorldPos) * newScale;
    grid.SetRenderOffset(newOffset);
  }

  bool SaveGrid() {
    auto dialogResult = OpenSaveDialog();
    if (dialogResult.has_value()) {
      curFilename = dialogResult.value();
      grid.Save(curFilename.string());
      unsavedChanges = false;
      sAppName = std::format(appNameBaseFmt, curFilename.filename().string());
      return true;
    }
    return false;
  }

  bool LoadGrid() {
    auto dialogResult = OpenLoadDialog();
    if (dialogResult.has_value()) {
      curFilename = dialogResult.value();
      grid.Load(curFilename.string());
      unsavedChanges = false;
      sAppName = std::format(appNameBaseFmt, curFilename.filename().string());
      return true;
    }
    return false;
  }

  std::optional<std::string> OpenSaveDialog() {
    constexpr nfdfilteritem_t filterItem[] = {
        {"Grid files", "grid"},
    };
    NFD::Init();
    NFD::UniquePath resultPath;
    auto curDir = std::filesystem::path(curFilename).parent_path().string();
    auto result = NFD::SaveDialog(resultPath, filterItem, 1, curDir.c_str());
    if (result != nfdresult_t::NFD_OKAY) return std::nullopt;
    std::string filename = resultPath.get();
    if (filename == "") return std::nullopt;
    NFD::Quit();
    return std::optional(filename);
  }

  std::optional<std::string> OpenLoadDialog() {
    constexpr nfdfilteritem_t filterItem[] = {
        {"Grid files", "grid"},
    };
    NFD::Init();
    NFD::UniquePath resultPath;
    auto curDir = std::filesystem::path(curFilename).parent_path().string();
    auto result = NFD::OpenDialog(resultPath, filterItem, 1, curDir.c_str());
    if (result != nfdresult_t::NFD_OKAY) return std::nullopt;
    std::string filename = resultPath.get();
    if (filename == "") return std::nullopt;
    NFD::Quit();
    return std::optional(filename);
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
    JustifyBufferTiles();
    CalculateTileBufferBoxSize();
  }

  void PasteTiles(const olc::vi2d& pastePosition) {
    auto uniqueTiles =
        tileBuffer |
        std::views::transform([&](const auto& tile) { return tile->Clone(); });
    grid.SetSelection(pastePosition, uniqueTiles);

    unsavedChanges = true;
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
    unsavedChanges = true;
  }

  // Draw the current buffer tiles half transparent (preview)
  // TODO: We should move the scale variables out of the Grid, it has no right
  // to manage those
  void DrawTilePreviews(olc::vi2d highlightWorldPos) {
    if (!tileBuffer.empty() && !selectionActive) {
      // Then draw each tile with transparency
      for (const auto& tile : tileBuffer) {
        // Calculate position for preview
        auto previewPos = tile->GetPos() + highlightWorldPos;
        // Draw with transparency
        tile->Draw(this, grid.WorldToScreen(previewPos), grid.GetRenderScale(),
                   128);
      }
    }
  }

  void DrawHighlight(olc::vi2d highlightWorldPos) {
    if (tileBufferBoxSize == olc::vi2d(0, 0)) {
      return;
    }
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
            "Selection rectangle has negative size, this should never happen");
      }
      olc::vf2d posScreen = grid.WorldToScreenFloating(topLeft);
      olc::vf2d sizeScreen = {gridSize.x * grid.GetRenderScale(),
                              gridSize.y * grid.GetRenderScale()};
      SetDrawTarget(uiLayer);
      DrawRectDecal(posScreen, sizeScreen, highlightColor);
    } else {
      SetDrawTarget(uiLayer);
      DrawRectDecal(grid.WorldToScreenFloating(highlightWorldPos),
                    tileBufferBoxSize * grid.GetRenderScale(), highlightColor);
    }
  }

  uint64_t drawTimeMicros = 0;
  uint64_t simTimeMicros = 0;

  void DrawStatusString(olc::vi2d highlightWorldPos, int drawnTilesCount) {
    // Status string
    std::stringstream ss;
    ss << highlightWorldPos << "; "
       << " Buffer: ";

    if (tileBuffer.empty()) {
      ss << "Empty";
    } else if (tileBuffer.size() == 1) {
      ss << tileBuffer[0]->TileTypeName();
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
       << "Tiles: " << grid.GetTileCount() << " (" << drawnTilesCount
       << " visible"
       << ")" << "; "
       << "Updates: " << updatesPerTick << " per tick\n"
       << "CPU Draw Time: "
       << std::format("{:.3f} ms\n", drawTimeMicros / 1000.f)
       << "CPU Sim Time: ";
    if (paused) {
      ss << "Paused\n";
    } else {
      ss << std::format("{:.3f} ms\n", simTimeMicros / 1000.f);
    }

    ss << "Press ',' to increase and '.' to decrease speed";

    SetDrawTarget((uint8_t)(uiLayer & 0x0F));
    DrawString(olc::vi2d(0, 0), ss.str(), olc::BLACK, 2);
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
      Reset();
    } else if (command == "clear") {
      ConsoleClear();
    } else if (command == "new") {
      grid.Clear();
      ConsoleOut() << "Grid cleared" << std::endl;
    } else if (command == "help") {
      ConsoleOut() << "Available commands: exit, pause, reset, clear, new, "
                      "help, gettile, toggleconsole"
                   << std::endl;
    } else if (command.starts_with("gettile")) {
      // cut down into two args
      auto args = command.substr(8);
      auto pos = args.find(' ');
      if (pos == std::string::npos) {
        ConsoleOut() << "Usage: gettile <x> <y>" << std::endl;
        return true;
      }
      int x = std::stoi(args.substr(0, pos));
      int y = std::stoi(args.substr(pos + 1));
      auto tileOpt = grid.GetTile(olc::vi2d(x, y));
      if (tileOpt.has_value()) {
        ConsoleOut() << tileOpt.value()->GetTileInformation() << std::endl;
      }

    } else {
      ConsoleOut() << "Unknown command: " << command << std::endl;
    }
    return true;
  }

  // --- Main update loop ---
  bool OnUserUpdate(float fElapsedTime) override {
    // Handle window resizing
    auto curScreenSize = GetWindowSize() / GetPixelSize();
    if (grid.GetRenderWindow() != curScreenSize) {
#ifdef DEBUG
      std::cout << std::format("Window resized to {}x{}", curScreenSize.x,
                               curScreenSize.y)
                << std::endl;
#endif
      grid.Resize(curScreenSize);
      SetScreenSize(curScreenSize.x, curScreenSize.y);
    }

    // Simulation step
    accumulatedTime += fElapsedTime;
    if (accumulatedTime > updateInterval && !paused) {
      try {
        auto simStartTime = std::chrono::high_resolution_clock::now();
        updatesPerTick = grid.Simulate();
        auto simEndTime = std::chrono::high_resolution_clock::now();

        simTimeMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                            simEndTime - simStartTime)
                            .count();
      } catch (const std::runtime_error& e) {
#ifdef DEBUG
        ConsoleOut() << std::format("Simulation runtime error: {}", e.what())
                     << std::endl;
#endif
        paused = true;
        Reset();
        updatesPerTick = 0;  // Reset updates per tick on error
      } catch (const std::invalid_argument& e) {
        ConsoleOut() << std::format("Simulation invalid argument error: {}",
                                    e.what())
                     << std::endl;
        paused = true;
        Reset();
        updatesPerTick = 0;  // Reset updates per tick on error
      }
      accumulatedTime = 0;
    }

    // Calculate mouse position and highlight (always needed for drawing)
    auto [selTileXIndex, selTileYIndex] = controlManager.GetMousePosition();
    olc::vf2d hoverWorldPos =
        grid.ScreenToWorld(olc::vi2d(selTileXIndex, selTileYIndex));
    olc::vi2d highlightWorldPos = grid.AlignToGrid(hoverWorldPos);

    if (!unsavedChangesGui.IsEnabled()) {
      const auto& events = controlManager.ProcessInput();
      for (const auto& event : events) {
        HandleGameEvent(event, fElapsedTime, highlightWorldPos);
      }
    }
    auto guiResult = unsavedChangesGui.Update();

    // Draw grid and UI

    auto drawStartTime = std::chrono::high_resolution_clock::now();
    SetDrawTarget(uiLayer);
    Clear(olc::BLANK);
    int drawnTiles = grid.Draw(this);
    unsavedChangesGui.Draw();
    if (!unsavedChangesGui.IsEnabled()) {
      // Draw the other GUI elements
      if (paused) {
        DrawHighlight(highlightWorldPos);
        DrawTilePreviews(highlightWorldPos);
      }
      auto drawEndTime = std::chrono::high_resolution_clock::now();
      drawTimeMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                           drawEndTime - drawStartTime)
                           .count();
      DrawStatusString(highlightWorldPos, drawnTiles);
    }

    if (guiResult != MessageBoxGui::Result::Nothing) {
      switch (guiResult) {
        case MessageBoxGui::Result::Yes:
          if (!SaveGrid()) break;
          /* FALLTHRU */
        case MessageBoxGui::Result::No:
          // If engineRunning is false here, don't prompt to open another file
          if (engineRunning) {
            if (LoadGrid()) unsavedChanges = false;
          } else {
            unsavedChanges = false;
          }
          break;
        case MessageBoxGui::Result::Cancel:
          engineRunning = true;
          break;
        case MessageBoxGui::Result::Nothing:
          throw std::runtime_error("Unreachable");
      }
      // When we press an option on the GUI, right after, it'll place a tile
      // On the first frame after the GUI closes.
      // Fixed by clearing the buffer, for now at least.
      tileBuffer.clear();
      unsavedChangesGui.Disable();
    }
    return engineRunning | unsavedChangesGui.IsEnabled();
  }

  bool OnUserDestroy() override {
    if (unsavedChanges) {
      engineRunning = false;
      unsavedChangesGui.Enable();
      return false;  // Don't quit yet.
    }
    ConsoleCaptureStdOut(false);
    ConsoleOut() << std::endl;
    ConsoleOut() << "Exiting Electricity Simulator..." << std::endl;
    return true;
  }
};

// --- Static member initialization ---
const olc::vf2d Game::defaultRenderOffset = olc::vf2d(0, 0);

// TODO: Make a global sink variable which should be pixelGameEngine's console.
// --- Main entry point ---
int main(int argc, char** argv) {
  (void)argc;
  std::filesystem::path argPath;
  if (argv[1] != nullptr) {
    std::string arg = std::string(argv[1]);
    try {
      argPath = std::filesystem::canonical(arg);
    } catch (const std::filesystem::filesystem_error& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;  // Exit with error code
    }
  }

  Game game;
  if (game.Construct(640 * 2, 480 * 2, 1, 1, false, true, true, true)) {
    if (argPath.extension() == ".grid" && std::filesystem::exists(argPath)) {
      game.SetStartFile(argPath);
    }
    game.Start();
  }
  return 0;
}  // test comment
