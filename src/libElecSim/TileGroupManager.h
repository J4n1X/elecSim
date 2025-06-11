#pragma once

#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "GridTile.h"
#include "ankerl/unordered_dense.h"
#include "olcPixelGameEngine.h"

namespace ElecSim {
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

 private:
  // This is a map of all tiles, keyed by their position.
  // The value is a shared pointer to the tile.
  using TileMap =
      ankerl::unordered_dense::map<olc::vi2d, std::shared_ptr<GridTile>,
                                   PositionHash, PositionEqual>;
  using SimObjMap =
      ankerl::unordered_dense::map<olc::vi2d, std::shared_ptr<SimulationObject>,
                                   PositionHash, PositionEqual>;
  SimObjMap simulationObjects;

  // Helper functions for preprocessing
  bool HasOutputConnection(const std::shared_ptr<GridTile>& tile, const TileMap& tiles) const;
  bool HasDeterministicInputs(const std::shared_ptr<GridTile>& tile, const TileMap& tiles) const;
  bool IsValidStartTile(const std::shared_ptr<GridTile>& tile, const TileMap& tiles) const;
  std::vector<std::shared_ptr<GridTile>> FindInitialStartTiles(const TileMap& tiles) const;
  int CountInputsToTile(const std::shared_ptr<GridTile>& neighbor, const TileMap& tiles) const;
  std::shared_ptr<GridTile> FindInputterTile(
      const std::shared_ptr<GridTile>& tile,
      const TileMap& tiles,
      const ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>& pathVisited) const;
  void QueueNeighborsAsStartTiles(
      const std::shared_ptr<GridTile>& tile,
      const TileMap& tiles,
      std::queue<std::shared_ptr<GridTile>>& pendingStartTiles,
      const ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>& globalVisited) const;
  void ProcessDeterministicTileNeighbors(
      const std::shared_ptr<GridTile>& current,
      const TileMap& tiles,
      std::queue<std::shared_ptr<GridTile>>& pathQueue,
      std::vector<SimulationGroup::OutputTile>& outputTiles,
      std::queue<std::shared_ptr<GridTile>>& pendingStartTiles,
      const ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>& globalVisited) const;

  struct PathTraceResult {
    std::vector<std::shared_ptr<GridTile>> pathTiles;
    std::vector<SimulationGroup::OutputTile> outputTiles;
    ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>> pathVisited;
  };

  PathTraceResult TraceDeterministicPath(
      const std::shared_ptr<GridTile>& inputTile,
      const TileMap& tiles,
      std::queue<std::shared_ptr<GridTile>>& pendingStartTiles,
      const ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>& globalVisited) const;
  void CreateSimulationObject(
      const std::shared_ptr<GridTile>& inputTile,
      std::vector<std::shared_ptr<GridTile>> pathTiles,
      std::vector<SimulationGroup::OutputTile> outputTiles);
  void CoverRemainingTiles(const TileMap& tiles,
                          ankerl::unordered_dense::segmented_set<std::shared_ptr<GridTile>>& globalVisited);

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

}  // namespace ElecSim
