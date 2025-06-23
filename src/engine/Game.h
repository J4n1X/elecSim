#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Drawables.h"
#include "Grid.h"
#include "KeyState.h"
#include "SFML/Graphics.hpp"
#include "SFML/System/Clock.hpp"
#include "v2d.h"

class FPS {
 public:
  FPS() : mFrame(0), mFps(0) {}
  unsigned int getFPS() const { return mFps; }

  void update() {
    if (mClock.getElapsedTime().asSeconds() >= 1.f) {
      mFps = mFrame;
      mFrame = 0;
      mClock.restart();
    }
    ++mFrame;
  }

 private:
  unsigned int mFrame;
  unsigned int mFps;
  sf::Clock mClock;
};

class Game {
 public:
  Game();
  ~Game() = default;

  int Run(int argc, char* argv[]);

 private:
  void Initialize();
  void SaveGrid(std::string const& filename);
  void LoadGrid(std::string const& filename);
  void HandleEvents();
  void HandleInput();
  void HandleResize(const sf::Vector2u& newSize);
  void Update();
  void Render();

  sf::Vector2f AlignToGrid(const sf::Vector2f& pos) const;
  vi2d WorldToGrid(const sf::Vector2f& pos) const;

  // Window and rendering
  sf::RenderWindow window;
  sf::View gridView;
  sf::View guiView;
  constexpr static std::string windowTitle = std::string("ElecSim");

  // Game state
  std::string gridFilename; 
  ElecSim::Grid grid;
  Engine::Highlighter highlighter;
  std::vector<std::unique_ptr<Engine::TileDrawable>> renderables;

  // Input handling
  Engine::KeyState keysHeld;
  float mouseWheelDelta;
  sf::Vector2i panStartPos;
  sf::Vector2f cameraVelocity;

  // Camera and zoom
  float zoomFactor;
  static constexpr float defaultZoomFactor = 32.f;

  // Window settings
  static constexpr vu2d initialWindowSize = vi2d(1280, 960);

  // UI
  FPS fpsTracker;
  sf::Font font;
  sf::Text text;
  sf::Vector2f mousePos;
};
