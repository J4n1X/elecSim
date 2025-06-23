#include "Game.h"

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

  text.setCharacterSize(24);
  text.setFillColor(sf::Color::Black);
  text.setPosition(
      sf::Vector2f(10.f, 10.f));  // Position text at top-left with margin

  if (!font.openFromMemory(BAHNSCHRIFT_TTF, BAHNSCHRIFT_TTF_len)) {
    std::cerr << "Error: Could not load font from memory.\n";
    throw std::runtime_error("Failed to load font");
  }

  keysHeld.reset();
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

  // Regenerate renderables from grid tiles
  renderables =
      grid.GetTiles() | std::views::values |
      std::views::transform(
          [](const auto& tile) { return Engine::CreateTileDrawable(tile); }) |
      std::ranges::to<std::vector<std::unique_ptr<Engine::TileDrawable>>>();
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
  while (const std::optional event = window.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      window.close();
    }

    // Handle keyboard press events
    if (auto keyDownEvent = event->getIf<sf::Event::KeyPressed>()) {
      keysHeld.setPressed(keyDownEvent->code);
    }

    if (auto keyUpEvent = event->getIf<sf::Event::KeyReleased>()) {
      keysHeld.setReleased(keyUpEvent->code);
    }

    if (auto mouseWheelEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
      mouseWheelDelta = mouseWheelEvent->delta;
    }
    if (auto mouseButtonEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
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
  if (keysHeld[Key::C]) {
    ResetViews();
    keysHeld.setReleased(Key::C);  // Disable the key after resetting
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

  // Draw UI
  text.setString(
      std::format("FPS: {}; Mouse Position: ({:.2f}, {:.2f}); Zoom level: "
                  "{:.2f}; Camera Velocity: ({:.2f}, {:.2f})\n",
                  fpsTracker.getFPS(), mousePos.x, mousePos.y, zoomFactor,
                  cameraVelocity.x, cameraVelocity.y));
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
}  // namespace Engine