#pragma once

#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>
#include <type_traits>

#include "GridTileTypes.h"  // Include this for derived tile types
#include "ankerl/unordered_dense.h"
#include "olcPixelGameEngine.h"


namespace ElecSim {
// TODO: Solve the tile lookup speed problem or find a way around it.
class Grid {
 private:  struct SignalEdge {
    olc::vi2d sourcePos;
    olc::vi2d targetPos;
    bool operator==(const SignalEdge& other) const;
  };
  struct SignalEdgeHash {
    using is_avalanching = void;
    std::size_t operator()(const SignalEdge& edge) const;
  };
  // Hash functor for olc::vi2d to use in unordered sets/maps
  struct PositionHash {
    using is_avalanching = void;
    std::size_t operator()(const olc::vi2d& pos) const;
  };

  // Equals functor for olc::vi2d
  struct PositionEqual {
    bool operator()(const olc::vi2d& lhs, const olc::vi2d& rhs) const;
  };

  using TileField = ankerl::unordered_dense::map<olc::vi2d, std::shared_ptr<GridTile>,
                                       PositionHash, PositionEqual>;

  olc::Pixel backgroundColor = olc::BLUE;
  olc::vi2d renderWindow;

  int uiLayer;
  int gameLayer;
  uint32_t currentTick = 0;  // Current game tick (used by emitters)

  TileField tiles;
  std::vector<std::weak_ptr<GridTile>> emitters;

  // Using a segmented set here because we are inserting a lot of things
  ankerl::unordered_dense::segmented_set<SignalEdge, SignalEdgeHash> currentTickVisitedEdges;
  std::queue<UpdateEvent> updateQueue;

  float renderScale;
  olc::vf2d renderOffset = {0.0f, 0.0f};  // Offset for rendering

  void ProcessUpdateEvent(const UpdateEvent& updateEvent);
  olc::vi2d TranslatePosition(olc::vi2d pos, Direction dir) const;

 public:
  Grid(olc::vi2d size, float renderScale, olc::vi2d renderOffset, int uiLayer,
       int gameLayer);
  Grid(int size_x, int size_y, float renderScale, olc::vi2d renderOffset,
       int uiLayer, int gameLayer)
      : Grid(olc::vi2d(size_x, size_y), renderScale, renderOffset, uiLayer,
             gameLayer) {}
  Grid() = default;
  ~Grid() = default;

  // Core simulation functions
  void QueueUpdate(std::shared_ptr<GridTile> tile, const SignalEvent& event);
  int Simulate();
  void ResetSimulation();

  // Rendering
  int Draw(olc::PixelGameEngine* renderer);  // returns amount of tiles drawn

  // Grid manipulation
  void EraseTile(olc::vi2d pos) { tiles.erase(pos); }
  void EraseTile(int x, int y) { EraseTile(olc::vi2d(x, y)); }

  void SetTile(olc::vf2d pos, std::unique_ptr<GridTile> tile, bool emitter);
  void SetSelection(olc::vi2d startPos, olc::vi2d endPos);

  // Utility functions
  olc::vf2d WorldToScreenFloating(const olc::vf2d& pos);
  olc::vi2d WorldToScreen(const olc::vf2d& pos);
  olc::vf2d ScreenToWorld(const olc::vi2d& pos);
  olc::vi2d AlignToGrid(const olc::vf2d& pos);
  olc::vf2d CenterOfSquare(const olc::vf2d& pos);

  // Getters
  olc::vi2d const& GetRenderWindow() { return renderWindow; }
  olc::vf2d const& GetRenderOffset() { return renderOffset; }
  float GetRenderScale() { return renderScale; }
  std::optional<std::shared_ptr<GridTile> const> GetTile(olc::vi2d pos);
  std::optional<std::shared_ptr<GridTile> const> GetTile(int x, int y) {
    return GetTile(olc::vi2d(x, y));
  }
  std::vector<std::weak_ptr<GridTile>> GetSelection(olc::vi2d startPos,
                                                    olc::vi2d endPos);
  std::size_t GetTileCount() { return tiles.size(); }

  // Configuration
  void Resize(olc::vi2d newSize) { renderWindow = newSize; }
  void Resize(int newWidth, int newHeight) {
    Resize(olc::vi2d(newWidth, newHeight));
  }
  void Clear() {
    tiles.clear();
    emitters.clear();
    ResetSimulation();
  }
  void SetRenderOffset(olc::vf2d newOffset) { renderOffset = newOffset; }
  void SetRenderScale(float newScale) { renderScale = newScale; }

  // Save/load
  void Save(const std::string& filename);
  void Load(const std::string& filename);
};

}  // namespace ElecSim