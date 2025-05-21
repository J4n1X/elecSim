#pragma once

#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include "GridTileTypes.h"
#include "olcPixelGameEngine.h"

class Grid {
 private:
  using TileField = std::map<olc::vi2d, std::shared_ptr<GridTile>>;

  olc::Pixel backgroundColor = olc::BLUE;
  olc::Pixel highlightColor = olc::RED;
  olc::vi2d renderWindow;

  int uiLayer;
  int gameLayer;
  int currentTick = 0;  // Current game tick (used by emitters)

  TileField tiles;
  std::vector<std::weak_ptr<GridTile>> emitters;
  std::priority_queue<UpdateEvent> updateQueue;

  float renderScale;
  olc::vf2d renderOffset = {};

  void ProcessSignalEvent(const SignalEvent& event);
  olc::vi2d TranslatePosition(olc::vi2d pos, Direction dir) const;

 public:
  Grid(olc::vi2d size, float renderScale, olc::vi2d renderOffset, int uiLayer,
       int gameLayer);
  Grid(int size_x, int size_y, float renderScale, olc::vi2d renderOffset,
       int uiLayer, int gameLayer)
      : Grid(olc::vi2d(size_x, size_y), renderScale, renderOffset, uiLayer,
             gameLayer) {}
  Grid() : Grid(0, 0, 0, olc::vi2d(0, 0), 0, 0) {};
  ~Grid() = default;

  // Core simulation functions
  void QueueUpdate(std::shared_ptr<GridTile> tile, const SignalEvent& event,
                   int priority = 0);
  int Simulate();
  void ResetSimulation();

  // Rendering
  int Draw(olc::PixelGameEngine* renderer,
           olc::vf2d* highlightPos);  // returns amount of tiles drawn

  // Grid manipulation
  void EraseTile(olc::vi2d pos) { tiles.erase(pos); }
  void EraseTile(int x, int y) { EraseTile(olc::vi2d(x, y)); }

  void SetTile(olc::vf2d pos, std::shared_ptr<GridTile> tile, bool emitter) {
    tiles.insert_or_assign(pos, tile);
    if (emitter) {
      emitters.push_back(tile);
    }
  }

  // Utility functions
  olc::vf2d WorldToScreenFloating(const olc::vf2d& pos);
  olc::vi2d WorldToScreen(const olc::vf2d& pos);
  olc::vf2d ScreenToWorld(const olc::vi2d& pos);
  olc::vf2d AlignToGrid(const olc::vf2d& pos);
  olc::vf2d CenterOfSquare(const olc::vf2d& pos);

  // Getters
  olc::vi2d const& GetRenderWindow() { return renderWindow; }
  olc::vf2d const& GetRenderOffset() { return renderOffset; }
  float GetRenderScale() { return renderScale; }
  std::optional<std::shared_ptr<GridTile> const> GetTile(olc::vi2d pos);
  std::optional<std::shared_ptr<GridTile> const> GetTile(int x, int y) {
    return GetTile(olc::vi2d(x, y));
  }
  std::size_t GetTileCount() { return tiles.size(); }

  // Configuration
  void Resize(olc::vi2d newSize) { renderWindow = newSize; }
  void Resize(int newWidth, int newHeight) {
    Resize(olc::vi2d(newWidth, newHeight));
  }
  void SetRenderOffset(olc::vf2d newOffset) { renderOffset = newOffset; }
  void SetRenderScale(float newScale) { renderScale = newScale; }

  // Save/load
  void Save(const std::string& filename = "grid.bin");
  void Load(const std::string& filename);
};