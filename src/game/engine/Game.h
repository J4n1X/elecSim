#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Drawables.h"
#include "Grid.h"
#include "GridTileTypes.h"
#include "KeyState.h"
#include "MouseState.h"
#include "SFML/Graphics.hpp"
#include "SFML/System/Clock.hpp"
#include "v2d.h"
#include "TileChunkManager.h"
#include "TilePreviewRenderer.h"

namespace Engine {

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

class FrameTime {
 public:
  FrameTime() : frameTime(0.f) {}

  void update() {
    sf::Time deltaTime = clock.restart();
    frameTime = deltaTime.asSeconds();
  }

  float getFrameTime() const { return frameTime; }

 private:
  sf::Clock clock;
  float frameTime;
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
  void ResetViews();
  void HandleEvents();
  void HandleInput();
  void HandleResize(const sf::Vector2u& newSize);
  void Update();
  void Render();  
  [[nodiscard]]sf::Vector2f AlignToGrid(const sf::Vector2f& pos) const;
  [[nodiscard]]ElecSim::vi2d WorldToGrid(const sf::Vector2f& pos) const noexcept;
  [[nodiscard]]sf::Vector2f GridToWorld(const ElecSim::vi2d& gridPos) const noexcept;

  // Updates the tile preview at the given position

  void InitChunks();

  // Tile manipulation methods
  void CreateBrushTile();
  void CalculateTileBufferBoxSize();
  void JustifyBufferTiles();
  void RotateBufferTiles();
  void ClearBuffer();
  void CopyTiles(const ElecSim::vi2d& startIndex, const ElecSim::vi2d& endIndex);
  void PasteTiles(const ElecSim::vi2d& pastePosition);
  void CutTiles(const ElecSim::vi2d& startIndex, const ElecSim::vi2d& endIndex);
  void DeleteTiles(const ElecSim::vi2d& position);

  // Window and rendering
  sf::RenderWindow window;
  sf::View gridView;
  sf::View guiView;
  constexpr static std::string_view windowTitle = "ElecSim";  // Game state
  std::string gridFilename;
  ElecSim::Grid grid;
  Highlighter highlighter;

  TileTextureAtlas textureAtlas; 
  TileChunkManager chunkManager;
  TilePreviewRenderer previewRenderer;  // Used for rendering tile previews


  // Game state
  bool paused = true; // Simulation state
  float tps = 8.f;  // Ticks per second for simulation
  float lastTickElapsedTime = 0.f;  // Time elapsed since last tick
  ElecSim::Grid::SimulationResult lastSimulationResult;  // Result of the last simulation

  // Tile manipulation
  bool selectionActive = false;
  bool unsavedChanges = false;
  ElecSim::vi2d selectionStartIndex = {0, 0};  // Start tile index for selection
  std::vector<std::unique_ptr<ElecSim::GridTile>> tileBuffer;  // Selected tiles for operations
  ElecSim::vi2d tileBufferBoxSize = {0, 0};  // Size of the tile buffer box
  int selectedBrushIndex = 1;
  ElecSim::Direction selectedBrushFacing = ElecSim::Direction::Top;

  // Input handling
  // TODO: Maybe we can bind these together.
  KeyState keysPressed;
  KeyState keysHeld;
  KeyState keysReleased;
  MouseState mousePressed;
  MouseState mouseHeld;
  MouseState mouseReleased;
  float mouseWheelDelta;
  sf::Vector2i panStartPos;
  sf::Vector2f cameraVelocity;

  // Camera and zoom
  static constexpr float defaultZoomFactor = 32.f;
  float zoomFactor;

  // Window settings
  static constexpr sf::Vector2u initialWindowSize = sf::Vector2u(1280, 960);

  // Time tracking systems
  FPS fpsTracker;
  FrameTime frameTimeTracker;

  // UI
  sf::Font font;
  sf::Text text;
  sf::Vector2f mousePos;


};
}  // namespace Engine