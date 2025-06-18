#include <ranges>
#include <type_traits>

#include "Drawables.h"
#include "Grid.h"
#include "SFML/Graphics.hpp"
#include "SFML/Graphics/Rect.hpp"
#include "SFML/System/Vector2.hpp"
#include "v2d.h"

constexpr const static vu2d initialWindowSize = vi2d(1280, 960);

// class Game {
// public:
//   Game() {
//     window.create(sf::VideoMode(ToSfmlVector(initialWindowSize)), "ElecSim");
//   }
//   [[nodiscard]] bool IsRunning() const noexcept{
//     return window.isOpen();
//   }
// private:
//   sf::Window window;
// };
sf::Vector2f AlignToGrid(const sf::Vector2f& pos) {
  return sf::Vector2f(
      std::floor(pos.x / Engine::BasicTileDrawable::DEFAULT_SIZE) *
          Engine::BasicTileDrawable::DEFAULT_SIZE,
      std::floor(pos.y / Engine::BasicTileDrawable::DEFAULT_SIZE) *
          Engine::BasicTileDrawable::DEFAULT_SIZE);
}

int main(int argc, char* argv[]) {
  sf::RenderWindow window(
      sf::VideoMode(Engine::ToSfmlVector(initialWindowSize)), "ElecSim");
  sf::FloatRect cullingBox = sf::FloatRect({0.f, 0.f}, {1280.f, 960.f});
  sf::View view(cullingBox);
  Engine::Highlighter highlighter {
    {{0.f, 0.f},
     {Engine::BasicTileDrawable::DEFAULT_SIZE,
      Engine::BasicTileDrawable::DEFAULT_SIZE}}};

    ElecSim::Grid grid{};
    sf::Vector2f cameraVelocity(0.f, 0.f);

    if (argc > 1) {
      grid.Load(argv[1]);
    }

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
          if (code == sf::Keyboard::Key::Escape) {
            window.close();
          }
          // WASD moves around the viewport
          else if (code == sf::Keyboard::Key::W) {
            cameraVelocity.y = -10.f;  // Move up
          } else if (code == sf::Keyboard::Key::S) {
            cameraVelocity.y = 10.f;  // Move down
          } else if (code == sf::Keyboard::Key::A) {
            cameraVelocity.x = -10.f;  // Move left
          } else if (code == sf::Keyboard::Key::D) {
            cameraVelocity.x = 10.f;  // Move right
          }
        }
        if (auto keyUpEvent = event->getIf<sf::Event::KeyReleased>()) {
          sf::Keyboard::Key code = keyUpEvent->code;
          // Stop moving when the key is released
          if (code == sf::Keyboard::Key::W || code == sf::Keyboard::Key::S) {
            cameraVelocity.y = 0.f;
          } else if (code == sf::Keyboard::Key::A ||
                     code == sf::Keyboard::Key::D) {
            cameraVelocity.x = 0.f;
          }
        }
      }
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

      window.display();
      // Update and render logic would go here
    }
  }