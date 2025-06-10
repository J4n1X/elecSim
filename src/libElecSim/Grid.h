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

#ifdef SIM_CACHING
// This class (at least in the future) contains all tiles and tilegroups,
// returning a shared pointer to a special SimObject that will only have one
// available function - ProcessSignal.
class TileGroupManager {
 public:
  class SimulationObject {
   public:
    virtual ~SimulationObject() = default;
    // This function will be called when the simulation is running.
    // It should return a vector of new signal events to be processed.
    virtual std::vector<SignalEvent> ProcessSignal(
        const SignalEvent& signal) = 0;
    virtual std::string GetObjectInfo() const = 0;
  };
 private:
  class SimulationTile : public SimulationObject {
   public:
    std::shared_ptr<GridTile> tile;

    SimulationTile(std::shared_ptr<GridTile> t) : tile(std::move(t)) {}
    std::string GetObjectInfo() const {
      std::string info = "SimulationTile:\n  ";
      info += tile->GetTileInformation();
      return info;
    }
    std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override {
      // Process the signal using the tile's ProcessSignal method
      auto newSignals = tile->ProcessSignal(signal);
      return newSignals;
    }
  };

  class SimulationGroup : public SimulationObject {
   public:
    struct OutputTile {
      std::shared_ptr<GridTile> tile;
      std::shared_ptr<GridTile> inputterTile;
    };

   private:
    std::shared_ptr<GridTile> inputTile;
    std::vector<std::shared_ptr<GridTile>> inbetweenTiles;
    std::vector<OutputTile> outputTiles;

   public:
    SimulationGroup(std::shared_ptr<GridTile> input,
                    std::vector<std::shared_ptr<GridTile>> inbetween,
                    std::vector<OutputTile> output)
        : inputTile(std::move(input)),
          inbetweenTiles(std::move(inbetween)),
          outputTiles(std::move(output)) {}
    std::string GetObjectInfo() const override;
    std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  };

  // This is a map of all tiles, keyed by their position.
  // The value is a shared pointer to the tile.
  using TileMap =
      ankerl::unordered_dense::map<olc::vi2d, std::shared_ptr<GridTile>,
                                   PositionHash, PositionEqual>;
  using SimObjMap =
      ankerl::unordered_dense::map<olc::vi2d, std::shared_ptr<SimulationObject>,
                                   PositionHash, PositionEqual>;
  SimObjMap simulationObjects;

 public:
  TileGroupManager() = default;
  void Clear() {
    simulationObjects.clear();
  }
  void PreprocessTiles(const TileMap& tiles);  // This will preprocess all tiles
                                               // and create simulation objects.
  std::optional<std::shared_ptr<SimulationObject>> const GetSimulationObject(
      const olc::vi2d& pos) {
    auto it = simulationObjects.find(pos);
    if (it != simulationObjects.end()) {
      return it->second;
    }
    return std::nullopt;  // No simulation object found for this position
  }
  ~TileGroupManager() = default;
};

#endif  // SIM_CACHING

// TODO: Solve the tile lookup speed problem or find a way around it.
class Grid {
 private:
  using TileField =
      ankerl::unordered_dense::map<olc::vi2d, std::shared_ptr<GridTile>,
                                   PositionHash, PositionEqual>;

  olc::Pixel backgroundColor = olc::BLUE;
  olc::vi2d renderWindow;

  int uiLayer;
  int gameLayer;
  uint32_t currentTick = 0;  // Current game tick (used by emitters)

  TileField tiles;
  #ifdef SIM_CACHING
  TileGroupManager tileManager;  // Tile manager for simulation caching
  #endif
  std::vector<std::weak_ptr<GridTile>> emitters;

  // Using a segmented set here because we are inserting a lot of things
  ankerl::unordered_dense::segmented_set<SignalEdge, SignalEdgeHash>
      currentTickVisitedEdges;
  std::queue<UpdateEvent> updateQueue;

  float renderScale;
  olc::vf2d renderOffset = {0.0f, 0.0f};  // Offset for rendering

  void ProcessUpdateEvent(const UpdateEvent& updateEvent);

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