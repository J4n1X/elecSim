#include <iostream>
#include <ranges>
#include <string>
#include <type_traits>

#include "Drawables.h"
#include "Grid.h"
#include "SFML/Graphics.hpp"
#include "SFML/System/Clock.hpp"
#include "v2d.h"

constexpr const static vu2d initialWindowSize = vi2d(1280, 960);

ElecSim::Grid grid{};
Engine::Highlighter highlighter{{{0.f, 0.f},
                                 {Engine::BasicTileDrawable::DEFAULT_SIZE,
                                  Engine::BasicTileDrawable::DEFAULT_SIZE}}};
std::array<bool, sf::Keyboard::KeyCount> keysHeld{};
float mouseWheelDelta = 0.f;

constexpr const float defaultZoomFactor = 32.f;  // Default zoom factor
float zoomFactor = defaultZoomFactor;            // Current zoom factor

sf::Vector2f AlignToGrid(const sf::Vector2f& pos) {
  return sf::Vector2f(std::floor(pos.x / Engine::TileDrawable::DEFAULT_SIZE) *
                          Engine::TileDrawable::DEFAULT_SIZE,
                      std::floor(pos.y / Engine::TileDrawable::DEFAULT_SIZE) *
                          Engine::TileDrawable::DEFAULT_SIZE);
}

vi2d WorldToGrid(const sf::Vector2f& pos) {
  auto aligned = AlignToGrid(pos);
  return vi2d(static_cast<int>(aligned.x / Engine::TileDrawable::DEFAULT_SIZE),
              static_cast<int>(aligned.y / Engine::TileDrawable::DEFAULT_SIZE));
}

void HandleInput(sf::RenderWindow& window, sf::View& view,
                 sf::Vector2f& cameraVelocity) {
  cameraVelocity = sf::Vector2f(0.f, 0.f);  // Reset camera velocity
  float moveFactor = 1.f / zoomFactor;  // Adjust movement speed based on zoom
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::W)]) {
    cameraVelocity.y = -32.f * moveFactor;  // Move up
  }
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::S)]) {
    cameraVelocity.y = 32.f * moveFactor;  // Move down
  }
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::A)]) {
    cameraVelocity.x = -32.f * moveFactor;  // Move left
  }
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::D)]) {
    cameraVelocity.x = 32.f * moveFactor;  // Move right
  }

  // Escape key closes the window
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::Escape)]) {
    window.close();
  }

  // Mouse wheel zooms in and out
  // TODO: Restore "Zoom to mouse" functionality
  if (mouseWheelDelta != 0.f) {
    float zoomDiff = 1.f + mouseWheelDelta * 0.1f;  // Adjust zoom sensitivity
    view.zoom(zoomDiff);
    zoomFactor /= zoomDiff;
    mouseWheelDelta = 0.f;  // Reset after applying
  }
}

int generate_texture_atlas() {
  std::cout << "Generating texture atlas...\n";
  // Create a render texture for the atlas
  const int tileSize =
      static_cast<int>(Engine::BasicTileDrawable::DEFAULT_SIZE);
  const int tilesPerRow =
      256 / tileSize /
      2;  // Arrange tiles in a 3x3 grid (we have 7 tiles, so 3x3 is sufficient)
  const int atlasSize = tileSize * 2 * tilesPerRow;

  sf::RenderTexture renderTexture;
  if (!renderTexture.resize({static_cast<unsigned int>(atlasSize),
                             static_cast<unsigned int>(atlasSize)})) {
    std::cerr << "Failed to create render texture!\n";
    return -1;
  }

  // Create instances of all tile types
  std::vector<std::shared_ptr<ElecSim::GridTile>> tiles;

  // Wire
  tiles.push_back(std::make_shared<ElecSim::WireGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Junction
  tiles.push_back(std::make_shared<ElecSim::JunctionGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Emitter
  tiles.push_back(std::make_shared<ElecSim::EmitterGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // SemiConductor
  tiles.push_back(std::make_shared<ElecSim::SemiConductorGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Button
  tiles.push_back(std::make_shared<ElecSim::ButtonGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Inverter
  tiles.push_back(std::make_shared<ElecSim::InverterGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Crossing
  tiles.push_back(std::make_shared<ElecSim::CrossingGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Clear the render texture with a transparent background
  renderTexture.clear(sf::Color::Transparent);

  // Create drawables and render each tile to the atlas
  for (size_t i = 0; i < tiles.size(); ++i) {
    auto drawable = Engine::CreateTileDrawable(tiles[i]);

    // Calculate position in the atlas grid
    int row = static_cast<int>(i / tilesPerRow);
    int col = static_cast<int>(i % tilesPerRow);
    float x = col * tileSize * 2;
    float y = row * tileSize;

    // Position the drawable
    drawable->setPosition(sf::Vector2f(x, y) + drawable->getOrigin());

    // Update the visual state and draw
    drawable->UpdateVisualState();
    renderTexture.draw(*drawable);

    // And once more for the activated state
    drawable->GetTile()->SetActivation(true);
    drawable->UpdateVisualState();
    drawable->setPosition(sf::Vector2f(x + tileSize, y) +
                          drawable->getOrigin());
    renderTexture.draw(*drawable);

    std::cout << "Rendered " << tiles[i]->TileTypeName() << " at (" << x << ", "
              << y << ")\n";
  }

  // Finish rendering
  renderTexture.display();

  // Save the texture to file
  sf::Texture atlasTexture = renderTexture.getTexture();
  sf::Image atlasImage = atlasTexture.copyToImage();

  if (atlasImage.saveToFile("tile_atlas.png")) {
    std::cout << "Texture atlas saved as 'tile_atlas.png'\n";
    std::cout << "Atlas size: " << atlasSize << "x" << atlasSize << " pixels\n";
    std::cout << "Tile size: " << tileSize << "x" << tileSize << " pixels\n";
    std::cout << "Tiles per row: " << tilesPerRow << "\n";

    // Print tile mapping for reference
    std::cout << "\nTile mapping:\n";
    for (size_t i = 0; i < tiles.size(); ++i) {
      int row = static_cast<int>(i / tilesPerRow);
      int col = static_cast<int>(i % tilesPerRow);
      std::cout << "  " << tiles[i]->TileTypeName() << ": (" << col << ", "
                << row << ")\n";
    }

  } else {
    std::cerr << "Failed to save texture atlas!\n";
    return -1;
  }

  return 0;
}

class FPS {
 public:
  FPS() : mFrame(0), mFps(0) {}
  unsigned int getFPS() const { return mFps; }

 private:
  unsigned int mFrame;
  unsigned int mFps;
  sf::Clock mClock;

 public:
  void update() {
    if (mClock.getElapsedTime().asSeconds() >= 1.f) {
      mFps = mFrame;
      mFrame = 0;
      mClock.restart();
    }

    ++mFrame;
  }
};

// Texture atlas generation main function
int main(int argc, char* argv[]) {
  // Check if user wants to generate atlas
  if (argc > 1 && std::string(argv[1]) == "--generate-atlas") {
    return generate_texture_atlas();
  }

  sf::Vector2f initialWindowSizeVec(static_cast<float>(initialWindowSize.x),
                                    static_cast<float>(initialWindowSize.y));

  sf::RenderWindow window(
      sf::VideoMode(Engine::ToSfmlVector(initialWindowSize)), "ElecSim");
  window.setVerticalSyncEnabled(true);
  window.setFramerateLimit(60);

  sf::View gridView(
      sf::FloatRect({0.f, 0.f}, initialWindowSizeVec / zoomFactor));
  sf::View guiView(sf::FloatRect({0.f, 0.f}, initialWindowSizeVec));

  sf::Vector2f cameraVelocity(0.f, 0.f);

  // FPS tracker
  FPS fpsTracker;

  sf::Font font("media/BAHNSCHRIFT.TTF");
  sf::Text text(font);
  text.setCharacterSize(24);
  text.setFillColor(sf::Color::Black);

  keysHeld.fill(false);  // Initialize all keys as not pressed
  if (argc > 1) {
    grid.Load(argv[1]);
  }

  // Spin up a whole bunch of drawables out of the grid tiles.
  // Real easy to do with ranges and views. Love C++23.
  auto renderables =
      grid.GetTiles() | std::views::values |
      std::views::transform(
          [](const auto& tile) { return Engine::CreateTileDrawable(tile); }) |
      std::ranges::to<std::vector<std::unique_ptr<Engine::TileDrawable>>>();

  while (window.isOpen()) {
    fpsTracker.update();

    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
      // Handle keyboard press events
      if (auto keyDownEvent = event->getIf<sf::Event::KeyPressed>()) {
        unsigned int code = static_cast<unsigned int>(keyDownEvent->code);
        if (code < 0 || code >= sf::Keyboard::KeyCount) {
          std::cerr << std::format(
              "Warning: An invalid key has been pressed, yielding code {}. "
              "Skipping.\n",
              code);
          continue;  // Skip invalid key codes
        } else {
          keysHeld[code] = true;
        }
      }
      if (auto keyUpEvent = event->getIf<sf::Event::KeyReleased>()) {
        unsigned int code = static_cast<unsigned int>(keyUpEvent->code);
        if (code > 0 || code < sf::Keyboard::KeyCount) {
          keysHeld[code] = false;
        }
      }
      if (auto mouseWheelEvent =
              event->getIf<sf::Event::MouseWheelScrolled>()) {
        mouseWheelDelta = mouseWheelEvent->delta;
      }
      if (auto resizeEvent = event->getIf<sf::Event::Resized>()) {
        // Update the gridView to match the new window size
        gridView.setSize(sf::Vector2f(resizeEvent->size) / zoomFactor);
        guiView.setSize(sf::Vector2f(
            resizeEvent->size));  // This prevents warping of the GUI
      }
    }
    HandleInput(window, gridView, cameraVelocity);

    if (cameraVelocity != sf::Vector2f(0.f, 0.f)) {
      gridView.move(cameraVelocity);
    }
    // Get mouse position and align it to the grid
    sf::Vector2f mousePos = AlignToGrid(
        window.mapPixelToCoords(sf::Mouse::getPosition(window), gridView));
    highlighter.setPosition(mousePos);

    window.setView(gridView);
    window.clear(sf::Color::Blue);

    sf::FloatRect viewBounds(gridView.getCenter() - gridView.getSize() / 2.f,
                             gridView.getSize());

    window.setView(gridView);
    auto viewables =
        renderables | std::views::filter([viewBounds](const auto& tile) {
          return viewBounds.findIntersection(tile->getGlobalBounds())
              .has_value();
        });
    for (const auto& tile : viewables) {
      tile->UpdateVisualState();
      window.draw(*tile);
    }
    window.draw(highlighter);

    text.setString(std::format(
        "FPS: {}; Mouse Position: ({:.2f}, {:.2f}); Zoom level: {:.2f}\n",
        fpsTracker.getFPS(), mousePos.x, mousePos.y, zoomFactor));
    window.setView(guiView);
    window.draw(text);

    window.display();
    // Update and render logic would go here
  }
  return 0;
}