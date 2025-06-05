#pragma once

#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <type_traits>
#include <vector>

#include "GridTileTypes.h"  // Include this for derived tile types
#include "ankerl/unordered_dense.h"
#include "olcPixelGameEngine.h"

namespace ElecSim {
// TODO: Solve the tile lookup speed problem or find a way around it.
class Grid {
 private:
  struct SignalEdge {
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

  struct DeterministicPath {
   private:
   // This could probably be another shared_ptr to a non-deterministic tile.
    struct PathEnd {
      olc::vi2d pos;
      size_t index;
      // This is what needs to be sent to the end. If not set, nothing happens.
      std::optional<SignalEvent> resultingEvent; 
    };

   public:
    olc::vi2d pathStart;            // Location of first determininistic tile
    std::vector<PathEnd> pathEnds;  // Location of last deterministic tile
    std::vector<std::shared_ptr<GridTile>> path;
    // If the value needs to be flipped when applying, it's true on this list.
    std::vector<bool> flippedStateList;

    static DeterministicPath Begin(olc::vi2d startPos) {
      return DeterministicPath{startPos, {}, {}, {}};
    }

    void AddTile(std::shared_ptr<GridTile> tile) {
      flippedStateList.push_back(tile->GetActivation());
      path.push_back(std::move(tile));
    }

    // Take the position of the tile right outside the path and the
    // Direction from which it is to be activated.
    void AddPathEnd(olc::vi2d endPos, std::optional<SignalEvent> endEvent) {
      pathEnds.push_back({endPos, path.size() - 1, endEvent});
    }

    std::vector<std::pair<olc::vi2d, SignalEvent>> Apply(bool enable) {
      using UpdatesContainer = std::vector<std::pair<olc::vi2d, SignalEvent>>;
      UpdatesContainer updates;
      for (size_t i = 0; i < path.size(); ++i) {
        path[i]->SetActivation(
            enable ? flippedStateList[i] : !flippedStateList[i]);
      }
      for (auto& pathEnd : pathEnds) {
        if(!pathEnd.resultingEvent.has_value()) continue;
        auto realResultingEvent = pathEnd.resultingEvent.value();
        realResultingEvent.isActive = enable ? realResultingEvent.isActive
                                             : !realResultingEvent.isActive;
        updates.emplace_back(
            pathEnd.pos,
            realResultingEvent);
      }
      return updates;
    }

    std::string GetPathInformation() const {
      std::stringstream ss;
      ss << std::format("Path Start: ({}, {})\n", pathStart.x, pathStart.y)
         << "Path Ends:\n";
      for (const auto& end : pathEnds) {
        ss << std::format("  - ({}, {})\n", end.pos.x, end.pos.y);
      }
      ss << "Path Tiles:\n";
      for (const auto& tile : path) {
        ss << tile->GetTileInformation() + "\n";
      }
      return ss.str();
    }
  };

  using TileField =
      ankerl::unordered_dense::map<olc::vi2d, std::shared_ptr<GridTile>,
                                   PositionHash, PositionEqual>;

  olc::Pixel backgroundColor = olc::BLUE;
  olc::vi2d renderWindow;

  int uiLayer;
  int gameLayer;
  uint32_t currentTick = 0;  // Current game tick (used by emitters)

  TileField tiles;
  ankerl::unordered_dense::map<olc::vi2d, std::vector<UpdateEvent>,
                               PositionHash, PositionEqual>
      deterministicPaths;  // Maps start positions to their paths
  std::vector<std::weak_ptr<GridTile>> emitters;

  // Using a segmented set here because we are inserting a lot of things
  ankerl::unordered_dense::segmented_set<SignalEdge, SignalEdgeHash>
      currentTickVisitedEdges;
  std::queue<UpdateEvent> updateQueue;

  float renderScale;
  olc::vf2d renderOffset = {0.0f, 0.0f};  // Offset for rendering

  std::vector<UpdateEvent> ChartDeterministicPath(const UpdateEvent& updateEvent);
  void ProcessUpdateEvent(const UpdateEvent& updateEvent);
  static olc::vi2d TranslatePosition(olc::vi2d pos, Direction dir);

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