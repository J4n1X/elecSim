#include "Game.h"

#include "GridTileTypes.h"
#include "nfd.hpp"

// This binary blob contains the "Bahnschrift" font that we are using.
#include <filesystem>
#include <format>
#include <iostream>
#include <ranges>

#include "BLOB_font.h"

std::optional<std::string> OpenSaveDialog() {
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

std::optional<std::string> OpenLoadDialog() {
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

void ZoomViewAt(sf::View& view, sf::Vector2i pixel,
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
      guiView(sf::FloatRect({0.f, 0.f}, sf::Vector2f(initialWindowSize))),
      grid{},
      highlighter{{{0.f, 0.f},
                   {Engine::BasicTileDrawable::DEFAULT_SIZE,
                    Engine::BasicTileDrawable::DEFAULT_SIZE}}},
      mouseWheelDelta(0.f),
      panStartPos(0, 0),
      cameraVelocity(0.f, 0.f),
      zoomFactor(defaultZoomFactor),
      text(font),
      mousePos(0.f, 0.f) {
  Initialize();
}

void Game::Initialize() {
  window.setVerticalSyncEnabled(true);
  window.setFramerateLimit(60);
  window.setKeyRepeatEnabled(false);

  text.setCharacterSize(24);
  text.setFillColor(sf::Color::Black);
  text.setPosition(
      sf::Vector2f(10.f, 10.f));  // Position text at top-left with margin

  if (!font.openFromMemory(BAHNSCHRIFT_TTF, BAHNSCHRIFT_TTF_len)) {
    std::cerr << "Error: Could not load font from memory.\n";
    throw std::runtime_error("Failed to load font");
  }
  keysHeld.reset();
  mouseHeld.reset();

  CreateBrushTile();  // Initialize tile buffer with default brush tile
}

void Game::SaveGrid(std::string const& filename) {
  grid.Save(filename);
  gridFilename = filename;
  window.setTitle(std::format("{} - {}", windowTitle, filename));
}

void Game::LoadGrid(std::string const& filename) {
  grid.Load(filename);
  gridFilename = filename;
  window.setTitle(std::format("{} - {}", windowTitle, filename));
  ResetViews();

  RegenerateRenderables();
}

void Game::ResetViews() {
  // Reset views and positions
  zoomFactor = defaultZoomFactor;
  gridView.setSize(sf::Vector2f(window.getSize()) / zoomFactor);
  gridView.setCenter(sf::Vector2f(window.getSize()) / zoomFactor / 2.f);
  guiView.setSize(sf::Vector2f(window.getSize()));
  guiView.setCenter(sf::Vector2f(window.getSize()) / 2.f);
}

int Game::Run(int argc, char* argv[]) {
  if (argc > 1) {
    LoadGrid(argv[1]);
  }

  while (window.isOpen()) {
    fpsTracker.update();
    HandleEvents();
    HandleInput();
    Update();
    Render();
  }

  return 0;
}

void Game::HandleEvents() {
  keysPressed.reset();
  keysReleased.reset();
  mousePressed.reset();
  mouseReleased.reset();

  while (const std::optional event = window.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      window.close();
    }

    // Handle keyboard press events
    if (auto keyDownEvent = event->getIf<sf::Event::KeyPressed>()) {
      keysPressed.setPressed(keyDownEvent->code);
      keysHeld.setPressed(keyDownEvent->code);
    }

    if (auto keyUpEvent = event->getIf<sf::Event::KeyReleased>()) {
      keysHeld.setReleased(keyUpEvent->code);
      keysReleased.setPressed(keyUpEvent->code);
    }
    if (auto mouseWheelEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
      mouseWheelDelta = mouseWheelEvent->delta;
    }

    if (auto mouseButtonEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
      mouseHeld.setPressed(mouseButtonEvent->button);
      mousePressed.setPressed(mouseButtonEvent->button);
    }

    if (auto mouseButtonEvent =
            event->getIf<sf::Event::MouseButtonReleased>()) {
      mouseHeld.setReleased(mouseButtonEvent->button);
      mouseReleased.setPressed(mouseButtonEvent->button);
    }

    if (auto resizeEvent = event->getIf<sf::Event::Resized>()) {
      HandleResize(resizeEvent->size);
    }
  }
}

void Game::HandleResize(const sf::Vector2u& newSize) {
  // Update the gridView to match the new window size
  gridView.setSize(sf::Vector2f(newSize) / zoomFactor);
  guiView.setSize(sf::Vector2f(newSize));

  // Reset the position of the gui view. No need to do it for the grid view.
  guiView.setCenter(sf::Vector2f(newSize) / 2.f);

  // Keep text at top-left with a small margin
  text.setPosition(sf::Vector2f(10.f, 10.f));
}

void Game::HandleInput() {
  using namespace sf::Keyboard;
  using namespace sf::Mouse;
  constexpr float baseMoveSpeed = 16.f;
  const float moveFactor = 1.f / zoomFactor;
  cameraVelocity = sf::Vector2f(0.f, 0.f);

  auto currentGridPos = WorldToGrid(mousePos);
  // Tile brush selection (number keys 1-7)
  for (int i = 1; i <= 7; ++i) {
    auto key = static_cast<Key>(static_cast<int>(Key::Num1) + (i - 1));
    if (keysHeld[key]) {
      selectedBrushIndex = i;
      CreateBrushTile();
      keysHeld.setReleased(key);
    }
  }

  // Tile manipulation controls
  if (keysHeld[Key::R]) {
    RotateBufferTiles();
    keysHeld.setReleased(Key::R);
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
    keysHeld.setReleased(Key::C);
    keysHeld.setReleased(Key::LControl);
    keysHeld.setReleased(Key::RControl);
  }

  // Cut Selection
  if ((keysHeld[Key::LControl] || keysHeld[Key::RControl]) &&
      keysHeld[Key::X]) {
    if (selectionActive) {
      CutTiles(selectionStartIndex, currentGridPos);
      selectionActive = false;
    }
    keysHeld.setReleased(Key::LControl);
    keysHeld.setReleased(Key::RControl);
    keysHeld.setReleased(Key::X);
  }

  // Paste tiles in buffer
  if ((keysHeld[Key::LControl] || keysHeld[Key::RControl]) &&
      keysPressed[Key::V]) {
    PasteTiles(currentGridPos);
    keysHeld.setReleased(Key::LControl);
    keysHeld.setReleased(Key::RControl);
  }

  if (keysHeld[Key::Z]) {
    ClearBuffer();
    keysHeld.setReleased(Key::Z);
  }

  // Mouse controls
  if (mouseHeld[Button::Left]) {
    if (!selectionActive) {
      if (currentGridPos != lastPlacedPos) {
        PasteTiles(currentGridPos);
        lastPlacedPos = currentGridPos;
      }
    }
  }

  if (mouseHeld[Button::Right]) {
    DeleteTiles(currentGridPos);
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
  if (keysHeld[Key::F]) {
    ResetViews();
    keysHeld.setReleased(Key::F);  // Disable the key after resetting
  }

  // Save/Load dialogues
  if (keysHeld[Key::F2]) {
    if (auto savePath = OpenSaveDialog()) {
      SaveGrid(*savePath);
    }
    // Disable the key to prevent accidental reloads
    keysHeld.setReleased(Key::F2);
  }
  if (keysHeld[Key::F3]) {
    if (auto loadPath = OpenLoadDialog()) {
      LoadGrid(*loadPath);
    }
    // Disable the key to prevent accidental reloads
    keysHeld.setReleased(Key::F3);
  }

  // Escape key closes the window
  if (keysHeld[Key::Escape]) {
    window.close();
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

  // Mouse position needs to be recalculated
  mousePos = AlignToGrid(
      window.mapPixelToCoords(sf::Mouse::getPosition(window), gridView));
  highlighter.setPosition(mousePos);
}

// --- Tile manipulation methods ---

void Game::CreateBrushTile() {
  // Clear the current buffer
  tileBuffer.clear();

  auto createTile = [this]() -> std::unique_ptr<ElecSim::GridTile> {
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

  if (!newTile) {
    return;  // No valid selection
  }

  // Set the facing direction
  newTile->SetFacing(selectedBrushFacing);
  tileBufferBoxSize = {1, 1};  // Default size for single tile

  // Add to buffer
  tileBuffer.push_back(std::move(newTile));
}

void Game::CalculateTileBufferBoxSize() {
  if (tileBuffer.empty()) {
    // This hides the highlight when no tiles are in the buffer
    tileBufferBoxSize = {0, 0};
    return;
  }

  vi2d minPos = {INT_MAX, INT_MAX};
  vi2d maxPos = {INT_MIN, INT_MIN};

  for (const auto& tile : tileBuffer) {
    vi2d pos = tile->GetPos();
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
  vi2d minPos =
      std::ranges::fold_right(tileBuffer, vi2d{INT_MAX, INT_MAX},
                              [](const auto& tile, const vi2d& acc) {
                                return vi2d{std::min(acc.x, tile->GetPos().x),
                                            std::min(acc.y, tile->GetPos().y)};
                              });
  for (auto& tile : tileBuffer) {
    auto clampedMinPos = vi2d{std::max(0, minPos.x), std::max(0, minPos.y)};
    auto newPos = tile->GetPos() - clampedMinPos;
    tile->SetPos(newPos);
  }
}

void Game::RotateBufferTiles() {
  vi2d maxPos = {INT_MIN, INT_MIN};

  // Rotate all tiles in the buffer to the next facing direction
  ElecSim::Direction newFacing =
      ElecSim::DirectionRotate(selectedBrushFacing, ElecSim::Direction::Right);

  if (selectedBrushFacing == newFacing) {
    throw std::runtime_error(
        "New facing should never be the same as the old facing");
  }
  selectedBrushFacing = newFacing;

  for (const auto& tile : tileBuffer) {
    vi2d pos = tile->GetPos();
    maxPos.x = std::max(maxPos.x, pos.x);
    maxPos.y = std::max(maxPos.y, pos.y);
  }
  // Calculate the far offset (must be the same in X and Y, so adjust if
  // needed)
  int farOffset = std::max(maxPos.x, maxPos.y);

  // Apply rotation to each tile
  for (auto& tile : tileBuffer) {
    vi2d rotatedRelPos = {0, 0};
    int newX = 0, newY = 0;
    vi2d relPos = tile->GetPos();
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
}

void Game::ClearBuffer() {
  tileBuffer.clear();
  CalculateTileBufferBoxSize();
}

void Game::CopyTiles(const vi2d& startIndex, const vi2d& endIndex) {
  // This sets the original facing direction to Top
  selectedBrushFacing = ElecSim::Direction::Top;

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

void Game::PasteTiles(const vi2d& pastePosition) {
  if (tileBuffer.empty()) return;

  for (const auto& tile : tileBuffer) {
    auto clonedTile = tile->Clone();
    auto newPos = tile->GetPos() + pastePosition;
    clonedTile->SetPos(newPos);
    grid.SetTile(newPos, std::move(clonedTile));
  }

  RegenerateRenderables();

  unsavedChanges = true;
}

void Game::CutTiles(const vi2d& startIndex, const vi2d& endIndex) {
  // First copy the tiles to buffer
  CopyTiles(startIndex, endIndex);

  // Then delete them from the grid
  // Ensure startIndex is actually the top-left corner
  vi2d topLeft = startIndex.min(endIndex);
  vi2d bottomRight = startIndex.max(endIndex);

  // Delete all tiles in the selection rectangle
  for (int y = topLeft.y; y <= bottomRight.y; y++) {
    for (int x = topLeft.x; x <= bottomRight.x; x++) {
      vi2d pos(x, y);
      grid.EraseTile(pos);
    }
  }

  RegenerateRenderables();

  unsavedChanges = true;
}

void Game::DeleteTiles(const vi2d& position) {
  grid.EraseTile(position);

  RegenerateRenderables();

  unsavedChanges = true;
  selectionActive = false;  // Clear selection after deletion
}

void Game::PlaceTile(const vi2d& position) {
  if (tileBuffer.empty()) return;

  // Place the first tile from the buffer (for single tile placement)
  auto clonedTile = tileBuffer[0]->Clone();
  clonedTile->SetPos(position);
  grid.SetTile(position, std::move(clonedTile));

  RegenerateRenderables();

  unsavedChanges = true;
}

void Game::Update() {
  if (cameraVelocity != sf::Vector2f(0.f, 0.f)) {
    gridView.move(cameraVelocity);
  }
}

void Game::Render() {
  window.setView(gridView);
  window.clear(sf::Color::Blue);

  sf::FloatRect viewBounds(gridView.getCenter() - gridView.getSize() / 2.f,
                           gridView.getSize());

  auto viewables =
      renderables | std::views::filter([viewBounds](const auto& tile) {
        return viewBounds.findIntersection(tile->getGlobalBounds()).has_value();
      });

  for (const auto& tile : viewables) {
    tile->UpdateVisualState();
    window.draw(*tile);
  }
  window.draw(highlighter);

  // Draw selection rectangle if active
  if (selectionActive) {
    auto currentGridPos = WorldToGrid(mousePos);
    vi2d topLeft = selectionStartIndex.min(currentGridPos);
    vi2d bottomRight = selectionStartIndex.max(currentGridPos);
    vi2d gridSize = bottomRight - topLeft + vi2d(1, 1);

    // Temporarily modify highlighter for selection display
    auto originalPos = highlighter.getPosition();
    auto originalSize = highlighter.getSize();

    highlighter.setPosition(
        sf::Vector2f(topLeft.x * Engine::TileDrawable::DEFAULT_SIZE,
                     topLeft.y * Engine::TileDrawable::DEFAULT_SIZE));
    highlighter.setSize(
        sf::Vector2f(gridSize.x * Engine::TileDrawable::DEFAULT_SIZE,
                     gridSize.y * Engine::TileDrawable::DEFAULT_SIZE));
    highlighter.setColor(sf::Color::Red);
    highlighter.setFillColor(
        sf::Color(255, 0, 0, 50));  // Semi-transparent red fill

    window.draw(highlighter);

    // Restore original highlighter state
    highlighter.setPosition(originalPos);
    highlighter.setSize(originalSize);
    highlighter.setColor(sf::Color(255, 0, 0, 128));
    highlighter.setFillColor(sf::Color::Transparent);
  }

  // Draw tile buffer preview (for paste preview)
  if (!tileBuffer.empty() && !selectionActive) {
    auto currentGridPos = WorldToGrid(mousePos);
    auto originalPos = highlighter.getPosition();
    auto originalSize = highlighter.getSize();

    for (const auto& tile : tileBuffer) {
      vi2d previewPos = tile->GetPos() + currentGridPos;
      highlighter.setPosition(
          sf::Vector2f(previewPos.x * Engine::TileDrawable::DEFAULT_SIZE,
                       previewPos.y * Engine::TileDrawable::DEFAULT_SIZE));
      highlighter.setSize(sf::Vector2f(Engine::TileDrawable::DEFAULT_SIZE,
                                       Engine::TileDrawable::DEFAULT_SIZE));
      highlighter.setColor(sf::Color::Green);
      highlighter.setFillColor(
          sf::Color(0, 255, 0, 100));  // Semi-transparent green fill

      // apply a tint to the colors
      window.draw(highlighter);
    }

    // Restore original highlighter state
    highlighter.setPosition(originalPos);
    highlighter.setSize(originalSize);
    highlighter.setColor(sf::Color(255, 0, 0, 128));
    highlighter.setFillColor(sf::Color::Transparent);
  }
  // Draw UI
  std::string brushName = "None";
  if (!tileBuffer.empty()) {
    brushName = std::string(tileBuffer[0]->TileTypeName());
  }

  std::string facingName =
      selectedBrushFacing == ElecSim::Direction::Top      ? "Top"
      : selectedBrushFacing == ElecSim::Direction::Right  ? "Right"
      : selectedBrushFacing == ElecSim::Direction::Bottom ? "Bottom"
                                                          : "Left";

  text.setString(std::format(
      "FPS: {}; Mouse Position: ({:.2f}, {:.2f}); Grid: ({}, {}); Zoom: "
      "{:.2f}\n"
      "Brush: {} ({}); Facing: {}; Buffer: {} tiles; Selection: {}\n",
      fpsTracker.getFPS(), mousePos.x, mousePos.y, WorldToGrid(mousePos).x,
      WorldToGrid(mousePos).y, zoomFactor, selectedBrushIndex, brushName,
      facingName, tileBuffer.size(), selectionActive ? "Active" : "None"));
  window.setView(guiView);
  window.draw(text);

  window.display();
}

sf::Vector2f Game::AlignToGrid(const sf::Vector2f& pos) const {
  return sf::Vector2f(std::floor(pos.x / Engine::TileDrawable::DEFAULT_SIZE) *
                          Engine::TileDrawable::DEFAULT_SIZE,
                      std::floor(pos.y / Engine::TileDrawable::DEFAULT_SIZE) *
                          Engine::TileDrawable::DEFAULT_SIZE);
}

vi2d Game::WorldToGrid(const sf::Vector2f& pos) const {
  auto aligned = AlignToGrid(pos);
  return vi2d(static_cast<int>(aligned.x / Engine::TileDrawable::DEFAULT_SIZE),
              static_cast<int>(aligned.y / Engine::TileDrawable::DEFAULT_SIZE));
}

void Game::RegenerateRenderables() {
  renderables =
      grid.GetTiles() | std::views::values |
      std::views::transform(
          [](const auto& tile) { return Engine::CreateTileDrawable(tile); }) |
      std::ranges::to<std::vector<std::unique_ptr<Engine::TileDrawable>>>();
}
}  // namespace Engine