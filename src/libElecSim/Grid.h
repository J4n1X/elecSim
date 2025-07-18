#pragma once

#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <type_traits>
#include <vector>

#include "GridTileTypes.h"  // Include this for derived tile types
#include "ankerl/unordered_dense.h"
#include "v2d.h"
#ifdef SIM_PREPROCESSING
#include "TileGroupManager.h"
#endif

namespace ElecSim {

struct SignalEdge {
  vi2d sourcePos;
  vi2d targetPos;
  bool operator==(const SignalEdge& other) const;
};
struct SignalEdgeHash {
  using is_avalanching = void;
  std::size_t operator()(const SignalEdge& edge) const;
};

class Grid {
 private:
  using TileField =
      ankerl::unordered_dense::map<vi2d, std::shared_ptr<GridTile>,
                                   PositionHash>;
  using VisitedEdgesSet =
      ankerl::unordered_dense::segmented_set<SignalEdge, SignalEdgeHash>;

  int currentTick = 0;        // Current game tick (used by emitters)
  bool fieldIsDirty = false;  // Flag to indicate if the field has been modified

  TileField tiles;
#ifdef SIM_PREPROCESSING
  TileGroupManager tileManager;  // Tile manager for simulation caching
#endif
  std::vector<std::weak_ptr<GridTile>> emitters;

  // Using a segmented set here because we are inserting a lot of things

  VisitedEdgesSet currentTickVisitedEdges;
  std::queue<UpdateEvent> updateQueue;

  void ProcessUpdateEvent(const UpdateEvent& updateEvent);

 public:
  struct SimulationResult {
    ankerl::unordered_dense::segmented_set<TileStateChange, TileStateChangeHash> affectedTiles;
    int updatesProcessed;
  };

  Grid() {};
  ~Grid() = default;

  /**
   * @brief Queues a tile update for processing in the next simulation step.
   * @param tile The tile to update
   * @param event The signal event that triggered the update
   */
  void QueueUpdate(std::shared_ptr<GridTile> tile,
                   const SignalEvent& event) noexcept;
  
  /**
   * @brief Runs the simulation until all queued updates are processed.
   * @return SimulationResult containing affected tiles and update count
   */
  SimulationResult Simulate();
  
  /**
   * @brief Resets the simulation state to initial conditions.
   */
  void ResetSimulation();

  // Grid manipulation
  void EraseTile(vi2d pos) {
    tiles.erase(pos);
    fieldIsDirty = true;
  }
  void EraseTile(int x, int y) { EraseTile(vi2d(x, y)); }

  // Sets a tile at the given position, overwriting the position is currently
  // has stored internally.
  void SetTile(vi2d pos, std::shared_ptr<GridTile> tile);
  // Expects a container of buffer tiles, which have coordinates relative to the
  // buffer system they are in. Supports any iterable container holding
  // std::unique_ptr<GridTile> or types convertible to it (e.g.,
  // std::shared_ptr<GridTile>, custom smart pointers)
  template <std::ranges::input_range Range>
    requires requires(std::ranges::range_value_t<Range> tile) {
      { tile->GetPos() } -> std::convertible_to<vi2d>;
      {
        std::unique_ptr<GridTile>(std::move(tile))
      } -> std::same_as<std::unique_ptr<GridTile>>;
    }
  void SetSelection(vi2d startPos, Range&& bufferTiles) {
    for (auto tile : bufferTiles) {
      auto pos = tile->GetPos() + startPos;
      std::unique_ptr<GridTile> converted = std::move(tile);
      SetTile(pos, std::move(converted));
    }
  }

  void InteractWithTile(vi2d pos) noexcept;

  // Utility functions
  [[nodiscard]] vi2d AlignToGrid(const vf2d& pos) noexcept;

  // Getters
  [[nodiscard]] std::optional<std::shared_ptr<GridTile> const> GetTile(
      vi2d pos);
  [[nodiscard]] std::optional<std::shared_ptr<GridTile> const> GetTile(int x,
                                                                       int y) {
    return GetTile(vi2d(x, y));
  }

  [[nodiscard]] const auto& GetTiles() const noexcept { return tiles; }

  std::vector<std::weak_ptr<GridTile>> GetSelection(vi2d startPos, vi2d endPos);
  std::size_t GetTileCount() { return tiles.size(); }

  // Configuration  }
  void Clear() {
    tiles.clear();
    emitters.clear();
    ResetSimulation();
  }

  // Save/load
  void Save(const std::string& filename);
  void Load(const std::string& filename);
};

}  // namespace ElecSim