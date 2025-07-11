#include "Game.h"

#include "GridTileTypes.h"
#include "TileChunk.h"
#include "nfd.hpp"
#include "imgui.h"
#include "imgui-SFML.h"

#include <filesystem>
#include <format>
#include <iostream>
#include <ranges>

static std::optional<std::string> OpenSaveDialog() {
  constexpr nfdfilteritem_t filterItem[] = {
      {"Grid files", "grid"},
  };
  NFD::Init();
  NFD::UniquePath resultPath;
  auto curDir = std::filesystem::current_path().string();
  auto result = NFD::SaveDialog(resultPath, filterItem, 1, curDir.c_str());
  if (result != nfdresult_t::NFD_OKAY) return std::nullopt;
  std::string filename = resultPath.get();
  if (filename == "") return std::nullopt;
  NFD::Quit();
  return std::optional(filename);
}

static std::optional<std::string> OpenLoadDialog() {
  constexpr nfdfilteritem_t filterItem[] = {
      {"Grid files", "grid"},
  };
  NFD::Init();
  NFD::UniquePath resultPath;
  auto curDir = std::filesystem::current_path().string();
  auto result = NFD::OpenDialog(resultPath, filterItem, 1, curDir.c_str());
  if (result != nfdresult_t::NFD_OKAY) return std::nullopt;
  std::string filename = resultPath.get();
  if (filename == "") return std::nullopt;
  NFD::Quit();
  return std::optional(filename);
}

static void ZoomViewAt(sf::View& view, sf::Vector2i pixel,
                       const sf::RenderWindow& window, float zoom) {
  const sf::Vector2f beforeCoord{window.mapPixelToCoords(pixel, view)};
  view.zoom(zoom);
  const sf::Vector2f afterCoord{window.mapPixelToCoords(pixel, view)};
  const sf::Vector2f offsetCoords{beforeCoord - afterCoord};
  view.move(offsetCoords);
}

namespace Engine {
Game::Game()
    : window(sf::VideoMode(initialWindowSize), windowTitle.data()),
      gridView(sf::FloatRect(
          {0.f, 0.f}, sf::Vector2f(initialWindowSize) / defaultZoomFactor)),
      grid{},
      highlighter{{{0.f, 0.f},
                   {Engine::TileDrawable::DEFAULT_SIZE,
                    Engine::TileDrawable::DEFAULT_SIZE}}},
      mouseWheelDelta(0.f),
      panStartPos(0, 0),
      cameraVelocity(0.f, 0.f),
      zoomFactor(defaultZoomFactor),
      mousePos(0.f, 0.f) {
  Initialize();
}

void Game::Initialize() {
  window.setVerticalSyncEnabled(true);
  window.setFramerateLimit(60);
  window.setKeyRepeatEnabled(false);

  if(!ImGui::SFML::Init(window)){
    throw std::runtime_error("Failed to initialize ImGui-SFML");
  }
  // We're not gonna let the user customize their windows across sessions.
  ImGui::GetIO().IniFilename = nullptr;  

  // Check if shaders are available
  if (!sf::Shader::isAvailable()) {
    std::cerr << "Warning: Shaders are not available on this system." << std::endl;
    std::cerr << "Preview rendering may not have transparency effects." << std::endl;
  }

  textureAtlas = TileTextureAtlas(static_cast<uint32_t>(defaultZoomFactor) * 4);
  chunkManager = TileChunkManager();
  previewRenderer.Initialize();

  keysHeld.Reset();
  mouseHeld.Reset();

  CreateBrushTile();  // Initialize tile buffer with default brush tile
}

void Game::Shutdown() {
  ImGui::SFML::Shutdown();
  if(window.isOpen()) [[unlikely]] {
    window.close();
  }
}

void Game::SaveGrid(std::string const& filename) {
  grid.Save(filename);
  gridFilename = filename;
  window.setTitle(std::format("{} - {}", windowTitle, filename));
  unsavedChanges = false;
}

void Game::LoadGrid(std::string const& filename) {
  grid.Load(filename);
  gridFilename = filename;
  window.setTitle(std::format("{} - {}", windowTitle, filename));
  ResetViews();
  unsavedChanges = false;

  InitChunks(); 
}

void Game::ResetViews() {
  // Reset views and positions
  zoomFactor = defaultZoomFactor;
  gridView.setSize(sf::Vector2f(window.getSize()) / zoomFactor);
  gridView.setCenter(sf::Vector2f(window.getSize()) / zoomFactor / 2.f);
}

void Game::ShowSaveDialog() {
  if (auto savePath = OpenSaveDialog()) {
    SaveGrid(*savePath);
  }
}

void Game::ShowLoadDialog() {
  if (auto loadPath = OpenLoadDialog()) {
    LoadGrid(*loadPath);
  }
}

void Game::AttemptQuit() {
  if (unsavedChanges) {
    unsavedChangesDialog.Show("You have unsaved changes. Do you want to save before quitting?");
    
    unsavedChangesDialog.SetOnSaveCallback([this]() {
      ShowSaveDialog();
      window.close();
    });
    
    unsavedChangesDialog.SetOnProceedCallback([this]() {
      window.close();
    });
    
    unsavedChangesDialog.SetOnCancelCallback([this]() noexcept {
      // Do nothing - just close the dialog
    });
  } else {
    window.close();
  }
}

int Game::Run(int argc, char* argv[]) {
  if (argc > 1) {
    LoadGrid(argv[1]);
  }

  while (window.isOpen()) {
    fpsTracker.update();
    frameTimeTracker.update();
    HandleEvents();
    HandleInput();
    Update();
    Render();
  }
  Shutdown();
  return 0;
}

void Game::HandleEvents() {
  keysPressed.Reset();
  keysReleased.Reset();
  mousePressed.Reset();
  mouseReleased.Reset();
  while (const std::optional event = window.pollEvent()) {
    ImGui::SFML::ProcessEvent(window, *event);

    if (event->is<sf::Event::Closed>()) {
      AttemptQuit();
    }

    // Block input events if dialog is visible or ImGui wants input (except for ImGui)
    if (!unsavedChangesDialog.IsVisible() && !ImGui::GetIO().WantCaptureKeyboard) {
      // Handle keyboard press events
      if (auto keyDownEvent = event->getIf<sf::Event::KeyPressed>()) {
        keysPressed.SetPressed(keyDownEvent->code);
        keysHeld.SetPressed(keyDownEvent->code);
      }

      if (auto keyUpEvent = event->getIf<sf::Event::KeyReleased>()) {
        keysHeld.SetReleased(keyUpEvent->code);
        keysReleased.SetPressed(keyUpEvent->code);
      }
      if (!ImGui::GetIO().WantCaptureMouse) {
        if (auto mouseWheelEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
          mouseWheelDelta = mouseWheelEvent->delta;
        }
        if (auto mouseButtonEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
          mouseHeld.SetPressed(mouseButtonEvent->button);
          mousePressed.SetPressed(mouseButtonEvent->button);
        }

        if (auto mouseButtonEvent =
                event->getIf<sf::Event::MouseButtonReleased>()) {
          mouseHeld.SetReleased(mouseButtonEvent->button);
          mouseReleased.SetPressed(mouseButtonEvent->button);
        }
      }
    }

    if (auto resizeEvent = event->getIf<sf::Event::Resized>()) {
      HandleResize(resizeEvent->size);
    }
  }
}

void Game::HandleResize(const sf::Vector2u& newSize) {
  // Update the gridView to match the new window size
  gridView.setSize(sf::Vector2f(newSize) / zoomFactor);
}

void Game::HandleInput() {
  using namespace sf::Keyboard;
  using namespace sf::Mouse;
  constexpr float baseMoveSpeed = 16.f;
  const float moveFactor = 1.f / zoomFactor;
  cameraVelocity = sf::Vector2f(0.f, 0.f);

  // Mouse position needs to be recalculated
  mousePos = AlignToGrid(
      window.mapPixelToCoords(sf::Mouse::getPosition(window), gridView));
  highlighter.setPosition(mousePos);
  
  // Block input if dialog is visible or ImGui wants input
  if (unsavedChangesDialog.IsVisible() || ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse) {
    return;
  }

  auto currentGridPos = WorldToGrid(mousePos);
  // Tile brush selection (number keys 1-7)
  for (int i = 1; i <= 7; ++i) {
    auto key = static_cast<Key>(static_cast<int>(Key::Num1) + (i - 1));
    if (keysHeld[key]) {
      selectedBrushIndex = i;
      CreateBrushTile();
      keysHeld.SetReleased(key);
    }
  }

  // Building mode controls
  if (paused) {
    // Tile manipulation controls
    if (keysHeld[Key::R]) {
      RotateBufferTiles();
      keysHeld.SetReleased(Key::R);
    }

    // Selection start/stop
    if (keysPressed[Key::LControl] || keysPressed[Key::RControl]) {
      selectionActive = true;
      selectionStartIndex = currentGridPos;
    }
    if (keysReleased[Key::LControl] || keysReleased[Key::RControl]) {
      selectionActive = false;
    }

    // Copy selection
    if ((keysHeld[Key::LControl] || keysHeld[Key::RControl]) &&
        keysHeld[Key::C]) {
      if (selectionActive) {
        CopyTiles(selectionStartIndex, currentGridPos);
      }
      keysHeld.SetReleased(Key::C);
      keysHeld.SetReleased(Key::LControl);
      keysHeld.SetReleased(Key::RControl);
    }

    // Cut Selection
    if ((keysHeld[Key::LControl] || keysHeld[Key::RControl]) &&
        keysHeld[Key::X]) {
      if (selectionActive) {
        CutTiles(selectionStartIndex, currentGridPos);
        selectionActive = false;
      }
      keysHeld.SetReleased(Key::LControl);
      keysHeld.SetReleased(Key::RControl);
      keysHeld.SetReleased(Key::X);
    }

    // Paste tiles in buffer
    if ((keysHeld[Key::LControl] || keysHeld[Key::RControl]) &&
        keysPressed[Key::V]) {
      PasteTiles(currentGridPos);
      keysHeld.SetReleased(Key::LControl);
      keysHeld.SetReleased(Key::RControl);
    }

    if (keysHeld[Key::Z]) {
      ClearBuffer();
      keysHeld.SetReleased(Key::Z);
    }

    // Mouse controls
    if (mouseHeld[Button::Left] && !selectionActive) {
      PasteTiles(currentGridPos);
    }

    if (mouseHeld[Button::Right]) {
      DeleteTiles(currentGridPos);
    }
  } else {  // Simulation controls
    if (mousePressed[Button::Left]) {
      // Interact with the tile under the mouse cursor
      grid.InteractWithTile(WorldToGrid(mousePos));
      unsavedChanges = true;
    }
    // TODO: A cool thing would be to select a tile with right click and then
    // show information about it.
  }

  // Camera controls
  if (keysHeld[Key::W] || keysHeld[Key::Up]) {
    cameraVelocity.y = -baseMoveSpeed * moveFactor;
  }
  if (keysHeld[Key::S] || keysHeld[Key::Down]) {
    cameraVelocity.y = baseMoveSpeed * moveFactor;
  }
  if (keysHeld[Key::A] || keysHeld[Key::Left]) {
    cameraVelocity.x = -baseMoveSpeed * moveFactor;
  }
  if (keysHeld[Key::D] || keysHeld[Key::Right]) {
    cameraVelocity.x = baseMoveSpeed * moveFactor;
  }
  if (keysPressed[Key::F]) {
    ResetViews();
  }

  // Middle mouse button pans the camera
  if (mouseHeld[Button::Middle]) {
    if (panStartPos == sf::Vector2i(0, 0)) {
      panStartPos = sf::Mouse::getPosition(window);
    }
    auto currentMousePos = sf::Mouse::getPosition(window);
    auto delta = currentMousePos - panStartPos;
    // Scale according to the zoom factor
    float dx =
        static_cast<float>(delta.x) / static_cast<float>(window.getSize().x);
    float dy =
        static_cast<float>(delta.y) / static_cast<float>(window.getSize().y);
    cameraVelocity = sf::Vector2f(dx, dy) * 2.f;
  } else {
    panStartPos = sf::Vector2i(0, 0);
  }

  // Mouse wheel zooms in and out
  if (mouseWheelDelta != 0.f) {
    float zoomDiff = 1.f + mouseWheelDelta * 0.1f;
    ZoomViewAt(gridView, sf::Mouse::getPosition(window), window, zoomDiff);
    zoomFactor /= zoomDiff;
    mouseWheelDelta = 0.f;
  }

  // Save/Load dialogues
  if (keysPressed[Key::F2]) {
    ShowSaveDialog();
  }
  if (keysPressed[Key::F3]) {
    if (unsavedChanges) {
      unsavedChangesDialog.Show("You have unsaved changes. Do you want to save before loading?");
      
      unsavedChangesDialog.SetOnSaveCallback([this]() {
        ShowSaveDialog();
        ShowLoadDialog();
      });
      
      unsavedChangesDialog.SetOnProceedCallback([this]() {
        ShowLoadDialog();
      });
      
      unsavedChangesDialog.SetOnCancelCallback([this]() noexcept {
        // Do nothing - just close the dialog
      });
    } else {
      ShowLoadDialog();
    }
  }

  // Escape key closes the window
  if (keysPressed[Key::Escape]) {
    AttemptQuit();
  }

  // Space starts or pauses the simulation
  if (keysPressed[Key::Space]) {
    paused = !paused;
    if (paused) {
      grid.ResetSimulation();
    }
  }

  // Comma and period adjust the ticks per second
  if (keysPressed[Key::Comma]) {  // FASTER!
    tps += 0.25f;
    keysPressed.SetReleased(Key::Comma);
  }
  if (keysPressed[Key::Period]) {  // Slow down...
    tps = std::max(0.1f, tps - 0.25f);
    keysPressed.SetReleased(Key::Period);
  }
}

// --- Tile manipulation methods ---

void Game::CreateBrushTile() {
  // Clear the current buffer
  tileBuffer.clear();

  auto createTile = [&]() -> std::unique_ptr<ElecSim::GridTile> {
    switch (selectedBrushIndex) {
      case 1:
        return std::make_unique<ElecSim::WireGridTile>();
      case 2:
        return std::make_unique<ElecSim::JunctionGridTile>();
      case 3:
        return std::make_unique<ElecSim::EmitterGridTile>();
      case 4:
        return std::make_unique<ElecSim::SemiConductorGridTile>();
      case 5:
        return std::make_unique<ElecSim::ButtonGridTile>();
      case 6:
        return std::make_unique<ElecSim::InverterGridTile>();
      case 7:
        return std::make_unique<ElecSim::CrossingGridTile>();
      default:
        return nullptr;
    }
  };

  auto newTile = createTile();

  if (!newTile) [[unlikely]] {
    return;  // No valid selection
  }

  // Set the facing direction
  newTile->SetFacing(selectedBrushFacing);
  tileBufferBoxSize = {1, 1};  // Default size for single tile

  // Add to buffer
  tileBuffer.push_back(std::move(newTile));
  
  // Update the preview with the new buffer
  previewRenderer.UpdatePreview(tileBuffer, textureAtlas);
}

void Game::CalculateTileBufferBoxSize() {
  if (tileBuffer.empty()) {
    // This hides the highlight when no tiles are in the buffer
    tileBufferBoxSize = {0, 0};
    return;
  }

  ElecSim::vi2d minPos = {INT_MAX, INT_MAX};
  ElecSim::vi2d maxPos = {INT_MIN, INT_MIN};

  for (const auto& tile : tileBuffer) {
    ElecSim::vi2d pos = tile->GetPos();
    minPos.x = std::min(minPos.x, pos.x);
    minPos.y = std::min(minPos.y, pos.y);
    maxPos.x = std::max(maxPos.x, pos.x);
    maxPos.y = std::max(maxPos.y, pos.y);
  }

  // Calculate the size of the bounding box
  tileBufferBoxSize.x = maxPos.x - minPos.x + 1;  // +1 to include the last tile
  tileBufferBoxSize.y = maxPos.y - minPos.y + 1;  // +1 to include the last tile
}

void Game::JustifyBufferTiles() {
  if (tileBuffer.empty()) return;

  // Find minimum position across all tiles
  ElecSim::vi2d minPos = {INT_MAX, INT_MAX};
  for (const auto& tile : tileBuffer) {
    ElecSim::vi2d pos = tile->GetPos();
    minPos.x = std::min(minPos.x, pos.x);
    minPos.y = std::min(minPos.y, pos.y);
  }

  // Adjust all tiles to make minPos the origin (0,0)
  for (auto& tile : tileBuffer) {
    tile->SetPos(tile->GetPos() - minPos);
  }
}

void Game::RotateBufferTiles() {
  ElecSim::vi2d maxPos = {INT_MIN, INT_MIN};

  // Rotate all tiles in the buffer to the next facing direction
  ElecSim::Direction newFacing =
      ElecSim::DirectionRotate(selectedBrushFacing, ElecSim::Direction::Right);

  if (selectedBrushFacing == newFacing) [[unlikely]] {
    throw std::runtime_error(
        "New facing should never be the same as the old facing");
  }
  selectedBrushFacing = newFacing;

  for (const auto& tile : tileBuffer) {
    ElecSim::vi2d pos = tile->GetPos();
    maxPos.x = std::max(maxPos.x, pos.x);
    maxPos.y = std::max(maxPos.y, pos.y);
  }
  // Calculate the far offset (must be the same in X and Y, so adjust if
  // needed)
  int farOffset = std::max(maxPos.x, maxPos.y);

  // Apply rotation to each tile
  for (auto& tile : tileBuffer) {
    ElecSim::vi2d rotatedRelPos = {0, 0};
    int newX = 0, newY = 0;
    ElecSim::vi2d relPos = tile->GetPos();
    newY = relPos.x;
    newX = farOffset - relPos.y;
    rotatedRelPos.x = std::abs(newX);
    rotatedRelPos.y = std::abs(newY);
    tile->SetPos(rotatedRelPos);
    tile->SetFacing(
        ElecSim::DirectionRotate(tile->GetFacing(), ElecSim::Direction::Right));
  }

  JustifyBufferTiles();
  CalculateTileBufferBoxSize();
  
  // Update the preview after rotation
  previewRenderer.UpdatePreview(tileBuffer, textureAtlas);
}

void Game::ClearBuffer() {
  tileBuffer.clear();
  CalculateTileBufferBoxSize();
  
  // Clear the preview as well
  previewRenderer.ClearPreview();
}

void Game::CopyTiles(const ElecSim::vi2d& startIndex, const ElecSim::vi2d& endIndex) {
  // This sets the original facing direction to Top
  selectedBrushFacing = ElecSim::Direction::Top;

  // Get all tiles in the selection area as weak pointers
  auto weakSelection = grid.GetSelection(startIndex, endIndex);

  // Clear the current buffer
  tileBuffer.clear();

  // Create actual copies (clones) of the tiles and store them in our buffer
  for (auto& weakTile : weakSelection) {
    if (weakTile.expired()) [[unlikely]]
      continue;
    if (auto tilePtr = weakTile.lock()) {
      auto retTile = tilePtr->Clone();
      retTile->SetPos(retTile->GetPos() - startIndex);  // Adjust position
      tileBuffer.push_back(std::move(retTile));
    }
  }
  JustifyBufferTiles();
  CalculateTileBufferBoxSize();
  
  // Update the preview with copied tiles
  previewRenderer.UpdatePreview(tileBuffer, textureAtlas);
}

void Game::PasteTiles(const ElecSim::vi2d& pastePosition) {
  if (tileBuffer.empty()) return;
  std::vector<ElecSim::GridTile*> placedTiles;
  for (const auto& tile : tileBuffer) {
    if (std::optional oldTile = grid.GetTile(pastePosition + tile->GetPos())) {
      if ((*oldTile)->GetTileType() == tile->GetTileType()) [[unlikely]] {
        continue;
      }
    }
    auto clonedTile = tile->Clone();
    auto newPos = tile->GetPos() + pastePosition;
    clonedTile->SetPos(newPos);
    auto tilePtr = std::move(clonedTile);
    placedTiles.push_back(tilePtr.get());
    grid.SetTile(newPos, std::move(tilePtr));
  }

  for(const auto& tile : placedTiles) {
    chunkManager.SetTile(tile, textureAtlas);           
  }

  unsavedChanges = true;
}

void Game::CutTiles(const ElecSim::vi2d& startIndex, const ElecSim::vi2d& endIndex) {
  // First copy the tiles to buffer
  CopyTiles(startIndex, endIndex);

  // Then delete them from the grid
  // Ensure startIndex is actually the top-left corner
  ElecSim::vi2d topLeft = startIndex.min(endIndex);
  ElecSim::vi2d bottomRight = startIndex.max(endIndex);

  // Delete all tiles in the selection rectangle
  std::vector<ElecSim::vi2d> tilesToErase;
  
  for (int y = topLeft.y; y <= bottomRight.y; y++) {
    for (int x = topLeft.x; x <= bottomRight.x; x++) {
      ElecSim::vi2d pos(x, y);
      // Check if there's a tile at this position
      if (grid.GetTile(pos)) {
        tilesToErase.push_back(pos);
        grid.EraseTile(pos);
      }
    }
  }
  
  // Erase from the chunk manager
  chunkManager.EraseTiles(tilesToErase);
  
  unsavedChanges = true;
}

void Game::DeleteTiles(const ElecSim::vi2d& position) {
  // Check if there's a tile at this position
  if (grid.GetTile(position)) {
    grid.EraseTile(position);
    chunkManager.EraseTile(position);
  }

  unsavedChanges = true;
}

void Game::Update() {
  ImGui::SFML::Update(window, frameTimeTracker.getTime());

  if (!paused) lastTickElapsedTime += frameTimeTracker.getFrameTime();
  if (cameraVelocity != sf::Vector2f(0.f, 0.f)) {
    gridView.move(cameraVelocity);
  }
  // Simulation step
  while (!paused && lastTickElapsedTime >= (1.f / tps)) {
    lastTickElapsedTime -= (1.f / tps);
    lastSimulationResult = grid.Simulate();
    // Update the visual state of tiles that changed in the simulation
    for (const auto& change : lastSimulationResult.affectedTiles) {
      if (auto tile = grid.GetTile(change.pos)) {
        chunkManager.SetTile(tile->get(), textureAtlas);
      }
    }
  }
}

void Game::Render() {
  window.setView(gridView);
  window.clear(sf::Color::Blue);

  chunkManager.RenderVisibleChunks(window, sf::RenderStates::Default, gridView, &textureAtlas.GetTexture());

  if (paused) {
    if (selectionActive) {
      auto currentGridPos = WorldToGrid(mousePos);
      ElecSim::vi2d topLeft = selectionStartIndex.min(currentGridPos);
      ElecSim::vi2d bottomRight = selectionStartIndex.max(currentGridPos);
      ElecSim::vi2d gridSize = bottomRight - topLeft + ElecSim::vi2d(1, 1);

      highlighter.setPosition(
          sf::Vector2f(topLeft.x * Engine::TileDrawable::DEFAULT_SIZE,
                       topLeft.y * Engine::TileDrawable::DEFAULT_SIZE));
      highlighter.setSize(
          sf::Vector2f(gridSize.x * Engine::TileDrawable::DEFAULT_SIZE,
                       gridSize.y * Engine::TileDrawable::DEFAULT_SIZE));
      window.draw(highlighter);
    }

    // Draw tile buffer preview (for paste preview)
    if (!tileBuffer.empty() && !selectionActive) {
      auto currentGridPos = WorldToGrid(mousePos);
      auto previewOffset = sf::Vector2f(static_cast<float>(currentGridPos.x),
                                        static_cast<float>(currentGridPos.y)) *
                           Engine::TileDrawable::DEFAULT_SIZE;
      
        // Just set the position for rendering - the preview content is updated
      // only when tiles change (in CreateBrushTile, RotateBufferTiles, CopyTiles)
      previewRenderer.setPosition(previewOffset);
      
      // Debug state printout can be enabled for troubleshooting if needed
      // previewRenderer.DebugPrintState();
      
      // Draw the preview
      window.draw(previewRenderer);
      
      // Draw the highlighter rectangle
      highlighter.setSize(sf::Vector2f(
          tileBufferBoxSize.x * Engine::TileDrawable::DEFAULT_SIZE,
          tileBufferBoxSize.y * Engine::TileDrawable::DEFAULT_SIZE));
      highlighter.setPosition(previewOffset);
      window.draw(highlighter);
    }
  }

  RenderStatusWindow();

  // Render the unsaved changes dialog if visible
  unsavedChangesDialog.Render();

  ImGui::SFML::Render(window);
  window.display();
}

sf::Vector2f Game::AlignToGrid(const sf::Vector2f& pos) const {
  return sf::Vector2f(std::floor(pos.x / Engine::TileDrawable::DEFAULT_SIZE) *
                          Engine::TileDrawable::DEFAULT_SIZE,
                      std::floor(pos.y / Engine::TileDrawable::DEFAULT_SIZE) *
                          Engine::TileDrawable::DEFAULT_SIZE);
}

ElecSim::vi2d Game::WorldToGrid(const sf::Vector2f& pos) const noexcept {
  auto aligned = AlignToGrid(pos);
  return ElecSim::vi2d(static_cast<int>(aligned.x / Engine::TileDrawable::DEFAULT_SIZE),
              static_cast<int>(aligned.y / Engine::TileDrawable::DEFAULT_SIZE));
}

sf::Vector2f Game::GridToWorld(const ElecSim::vi2d& gridPos) const noexcept {
  return sf::Vector2f(
      static_cast<float>(gridPos.x) * Engine::TileDrawable::DEFAULT_SIZE,
      static_cast<float>(gridPos.y) * Engine::TileDrawable::DEFAULT_SIZE);
}

void Game::InitChunks() {
  chunkManager.clear();
  
  // We can't directly use UpdateTiles here because Grid::GetTiles returns a map,
  // not a vector of unique_ptr<GridTile>. We would need to convert the data structure.
  // For now, using the direct approach
  for(const auto& tile : grid.GetTiles() | std::views::values) {
    chunkManager.SetTile(tile.get(), textureAtlas);
  }
  
  // TODO: Consider adding a method to Grid to get tiles as a vector of pointers,
  // which would allow us to use the UpdateTiles method here
}

// Hacky, but it does prevent storing yet another variable in the class
static constinit int lastUpdateCount = 0;
void Game::RenderStatusWindow() {
  // Prepare status text data
  std::string brushName = "None";
  if (!tileBuffer.empty()) {
    brushName = std::string(TileTypeToString(tileBuffer[0]->GetTileType()));
  }

  std::string facingName =
      selectedBrushFacing == ElecSim::Direction::Top      ? "Top"
      : selectedBrushFacing == ElecSim::Direction::Right  ? "Right"
      : selectedBrushFacing == ElecSim::Direction::Bottom ? "Bottom"
                                                          : "Left";

  if (lastSimulationResult.updatesProcessed > 0) {
    lastUpdateCount = lastSimulationResult.updatesProcessed;
  } else if (paused) {
    lastUpdateCount = 0;
  }

  // Create text strings to measure width
  std::vector<std::string> textLines = {
    std::format("FPS: {}", fpsTracker.getFPS()),
    std::format("Simulation: {}", paused ? "Paused" : "Running"),
    std::format("Grid Position: ({}, {})", WorldToGrid(mousePos).x, WorldToGrid(mousePos).y),
    std::format("TPS: {:.2f}", tps),
    std::format("Brush: {} ({})", selectedBrushIndex, brushName),
    std::format("Facing: {}", facingName),
    std::format("Buffer: {} tiles", tileBuffer.size()),
    std::format("Selection: {}", selectionActive ? "Active" : "None"),
    std::format("Total Tiles: {}", grid.GetTileCount()),
    std::format("Updates: {}", lastUpdateCount)
  };

  // Calculate maximum text width
  float maxTextWidth = 0.0f;
  for (const auto& line : textLines) {
    ImVec2 textSize = ImGui::CalcTextSize(line.c_str());
    maxTextWidth = std::max(maxTextWidth, textSize.x);
  }

  // Calculate window size that scales with screen but fits content
  ImVec2 displaySize = ImGui::GetIO().DisplaySize;
  float scaleFactor = std::min(displaySize.x / 1280.0f, displaySize.y / 960.0f); // Scale based on reference resolution
  scaleFactor = std::max(0.8f, std::min(scaleFactor, 2.0f)); // Clamp scaling
  
  // Calculate size with scaling
  float padding = 30.0f * scaleFactor;
  float windowWidth = maxTextWidth * scaleFactor + padding;
  float lineHeight = ImGui::GetTextLineHeight() * scaleFactor;
  float separatorHeight = ImGui::GetStyle().ItemSpacing.y * scaleFactor;
  float windowHeight = (lineHeight * textLines.size()) + 
                      (ImGui::GetStyle().ItemSpacing.y * scaleFactor * (textLines.size() - 1)) + 
                      (ImGui::GetStyle().WindowPadding.y * 2 * scaleFactor) + 
                      (separatorHeight * 2) + // Account for the 2 separators
                      (20.f * scaleFactor); // Extra padding for comfort
  
  ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2(10 * scaleFactor, 10 * scaleFactor), ImGuiCond_FirstUseEver);
  
  if (ImGui::Begin("Status", nullptr, ImGuiWindowFlags_NoResize)) {
    // Apply font scaling
    if (scaleFactor != 1.0f) {
      ImGui::SetWindowFontScale(scaleFactor);
    }
    ImGui::Text("FPS: %d", fpsTracker.getFPS());
    ImGui::Text("Simulation: %s", paused ? "Paused" : "Running");
    ImGui::Text("Grid Position: (%d, %d)", WorldToGrid(mousePos).x, WorldToGrid(mousePos).y);
    ImGui::Text("TPS: %.2f", tps);
    ImGui::Separator();
    ImGui::Text("Brush: %d (%s)", selectedBrushIndex, brushName.c_str());
    ImGui::Text("Facing: %s", facingName.c_str());
    ImGui::Text("Buffer: %zu tiles", tileBuffer.size());
    ImGui::Text("Selection: %s", selectionActive ? "Active" : "None");
    ImGui::Separator();
    ImGui::Text("Total Tiles: %zu", grid.GetTileCount());
    ImGui::Text("Updates: %d", lastUpdateCount);
    
    // Reset font scaling
    if (scaleFactor != 1.0f) {
      ImGui::SetWindowFontScale(1.0f);
    }
  }
  ImGui::End();
}


}  // namespace Engine