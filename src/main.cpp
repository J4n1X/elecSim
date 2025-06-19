#include <ranges>
#include <type_traits>

#include "Drawables.h"
#include "Grid.h"
#include "SFML/Graphics.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "SFML/System/Vector2.hpp"
#include "v2d.h"

constexpr const static vu2d initialWindowSize = vi2d(1280, 960);

ElecSim::Grid grid{};
Engine::Highlighter highlighter{{{0.f, 0.f},
                                 {Engine::BasicTileDrawable::DEFAULT_SIZE,
                                  Engine::BasicTileDrawable::DEFAULT_SIZE}}};
std::array<bool, sf::Keyboard::KeyCount> keysHeld{};
float mouseWheelDelta = 0.f;

sf::Vector2f AlignToGrid(const sf::Vector2f& pos) {
  return sf::Vector2f(
      std::floor(pos.x / Engine::BasicTileDrawable::DEFAULT_SIZE) *
          Engine::BasicTileDrawable::DEFAULT_SIZE,
      std::floor(pos.y / Engine::BasicTileDrawable::DEFAULT_SIZE) *
          Engine::BasicTileDrawable::DEFAULT_SIZE);
}

vi2d WorldToGrid(const sf::Vector2f& pos) {
  auto aligned = AlignToGrid(pos);
  return vi2d(
      static_cast<int>(aligned.x / Engine::BasicTileDrawable::DEFAULT_SIZE),
      static_cast<int>(aligned.y / Engine::BasicTileDrawable::DEFAULT_SIZE));
}

void HandleInput(sf::RenderWindow& window, sf::View& view,
                 sf::Vector2f& cameraVelocity) {
  cameraVelocity = sf::Vector2f(0.f, 0.f);  // Reset camera velocity
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::W)]) {
    cameraVelocity.y = -10.f;  // Move up
  }
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::S)]) {
    cameraVelocity.y = 10.f;  // Move down
  }
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::A)]) {
    cameraVelocity.x = -10.f;  // Move left
  }
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::D)]) {
    cameraVelocity.x = 10.f;  // Move right
  }

  // Escape key closes the window
  if (keysHeld[static_cast<int>(sf::Keyboard::Key::Escape)]) {
    window.close();
  }

  // Mouse wheel zooms in and out
  // TODO: Restore "Zoom to mouse" functionality
  if (mouseWheelDelta != 0.f) {
    sf::Vector2f viewSize = window.getView().getSize();
    float zoomFactor = 1.f + mouseWheelDelta * 0.1f;  // Adjust zoom sensitivity
    view.zoom(zoomFactor);
    mouseWheelDelta = 0.f;  // Reset after applying
  }
}

int main(int argc, char* argv[]) {
  sf::RenderWindow window(
      sf::VideoMode(Engine::ToSfmlVector(initialWindowSize)), "ElecSim");
  sf::FloatRect cullingBox = sf::FloatRect({0.f, 0.f}, {1280.f, 960.f});
  sf::View view(cullingBox);
  sf::Vector2f cameraVelocity(0.f, 0.f);

  // sf::Font font("arial.ttf");
  // sf::Text text(font);
  // text.setString("Hello, ElecSim!");
  // text.setCharacterSize(24);
  // text.setFillColor(sf::Color::Black);

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

  window.setVerticalSyncEnabled(true);
  window.setFramerateLimit(60);
  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
      // Handle keyboard press events
      if (auto keyDownEvent = event->getIf<sf::Event::KeyPressed>()) {
        sf::Keyboard::Key code = keyDownEvent->code;
        if (static_cast<int>(code) < 0 ||
            static_cast<int>(code) >= sf::Keyboard::KeyCount) {
          std::cerr << std::format(
              "Warning: An invalid key has been pressed, yielding code {}. "
              "Skipping.\n",
              static_cast<int>(code));
          continue;  // Skip invalid key codes
        } else {
          keysHeld[static_cast<int>(code)] = true;
        }
      }
      if (auto keyUpEvent = event->getIf<sf::Event::KeyReleased>()) {
        sf::Keyboard::Key code = keyUpEvent->code;
        if (static_cast<int>(code) > 0 ||
            static_cast<int>(code) < sf::Keyboard::KeyCount) {
          keysHeld[static_cast<int>(code)] = false;
        }
      }
      if (auto mouseWheelEvent =
              event->getIf<sf::Event::MouseWheelScrolled>()) {
        mouseWheelDelta = mouseWheelEvent->delta;
      }
    }
    HandleInput(window, view, cameraVelocity);

    if (cameraVelocity != sf::Vector2f(0.f, 0.f)) {
      view.move(cameraVelocity);
    }
    // Get mouse position and align it to the grid
    sf::Vector2f mousePos = AlignToGrid(
        window.mapPixelToCoords(sf::Mouse::getPosition(window), view));
    highlighter.setPosition(mousePos);

    window.setView(view);
    window.clear(sf::Color::Blue);

    sf::FloatRect viewBounds(view.getCenter() - view.getSize() / 2.f,
                             view.getSize());

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
    // window.draw(text);

    window.display();
    // Update and render logic would go here
  }
}