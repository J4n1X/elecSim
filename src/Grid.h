#pragma once

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "Tiles.h"
#include "olcPixelGameEngine.h"

// The entire grid. Handles drawing the playfield, gathering the neighbours for
// simulation and much more.

class Grid {
 private:
  using TileField = std::map<olc::vi2d, std::shared_ptr<GridTile>>;

  olc::Pixel backgroundColor = olc::BLUE;
  olc::Pixel highlightColor = olc::RED;
  olc::Pixel gridColor = olc::GREY;
  olc::vi2d renderWindow;

  int uiLayer;
  int gameLayer;

  TileField tiles;
  std::vector<std::weak_ptr<GridTile>> emitters;
  std::map<olc::vf2d, TileUpdateFlags> updates;

  // TODO: This should be in world space, not screen space...
  // render scale in pixels
  float renderScale;
  // render offset in pixels
  olc::vf2d renderOffset = {};

 public:
  Grid(olc::vi2d size, float renderScale, olc::vi2d renderOffset, int uiLayer,
       int gameLayer);
  // Grid(const Grid& grid);
  Grid(int size_x, int size_y, float renderScale, olc::vi2d renderOffset,
       int uiLayer, int gameLayer)
      : Grid(olc::vi2d(size_x, size_y), renderScale, renderOffset, uiLayer,
             gameLayer) {}
  Grid() : Grid(0, 0, 0, olc::vi2d(0, 0), 0, 0){};
  ~Grid() {}

  // Utility functions
  olc::vi2d WorldToScreen(const olc::vf2d& pos);
  olc::vf2d ScreenToWorld(const olc::vi2d& pos);
  void ZoomToMouse(const olc::vf2d& mouseWorldPosBefore,
                   const olc::vf2d& mouseWorldPosAfter);
  static olc::vi2d TranslateIndex(const olc::vf2d& index,
                                  const TileUpdateSide& side);
  olc::vf2d AlignToGrid(const olc::vf2d& pos);
  olc::vf2d CenterOfSquare(const olc::vf2d& squarePos);

  // Game Logic functions

  // TODO: Use std::optional instead of pointer
  void Draw(olc::PixelGameEngine* renderer, olc::vf2d* highlightIndex);
  void Simulate();
  void ResetSimulation();

  // Setters
  void Resize(olc::vi2d newSize) { renderWindow = newSize; }
  void Resize(int newWidth, int newHeight) {
    Resize(olc::vi2d(newWidth, newHeight));
  }
  void SetRenderOffset(olc::vf2d newOffset) { renderOffset = newOffset; }
  void SetRenderScale(float newScale) {
    renderScale = newScale > 0.0f ? newScale : renderScale;
  }

  void EraseTile(olc::vi2d pos) { tiles.erase(pos); }
  void EraseTile(int x, int y) { EraseTile(olc::vi2d(x, y)); }

  template <typename T>
  void SetTile(olc::vf2d pos, std::shared_ptr<T> tile, bool emitter) {
    tiles.insert_or_assign(pos, tile);
    if (emitter) {
      // add it to the list of tiles which will always be updated
      emitters.push_back(tile);
    }
    return;
  }
  template <typename T>
  void SetTile(float x, float y, std::shared_ptr<T> tile,
               bool emitter = false) {
    SetTile(tile, olc::vf2d(x, y));
  }

  // Getters
  olc::vi2d const& GetRenderWindow() { return renderWindow; }
  olc::vf2d const& GetRenderOffset() { return renderOffset; }
  float GetRenderScale() { return renderScale; }
  std::optional<std::shared_ptr<GridTile> const> GetTile(olc::vi2d pos) {
    auto tileIt = tiles.find(pos);
    return tileIt != tiles.end() ? std::optional{tileIt->second} : std::nullopt;
  }
  std::optional<std::shared_ptr<GridTile> const> GetTile(int x, int y) {
    return GetTile(olc::vi2d(x, y));
  }
};