#pragma once

#include "GridTile.h"

namespace ElecSim {

/**
 * @class WireGridTile
 * @brief Basic signal conductor that propagates signals in one direction.
 */
class WireGridTile : public DeterministicTile {
 public:
  explicit WireGridTile(vi2d pos = vi2d(0, 0),
               Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> PreprocessSignal(const SignalEvent incomingSignal) override;
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Wire; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

/**
 * @class JunctionGridTile
 * @brief Multi-directional signal splitter.
 */
class JunctionGridTile : public DeterministicTile {
 public:
  explicit JunctionGridTile(vi2d pos = vi2d(0, 0),
                   Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> PreprocessSignal(const SignalEvent incomingSignal) override;
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Junction; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

/**
 * @class EmitterGridTile
 * @brief Signal source that can be toggled and emits periodic signals.
 */
class EmitterGridTile : public LogicTile {
 protected:
  bool enabled;
  static constexpr int EMIT_INTERVAL = 3;
  int lastEmitTick;

 public:
  explicit EmitterGridTile(vi2d pos = vi2d(0, 0),
                  Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;
  void ResetActivation() override;
  bool ShouldEmit(int currentTick) const;

  bool IsEmitter() const override { return true; }
  TileType GetTileType() const override { return TileType::Emitter; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

/**
 * @class SemiConductorGridTile
 * @brief Logic gate that requires multiple inputs to activate.
 */
class SemiConductorGridTile : public LogicTile {
 protected:
  int internalState;  // bit 0: side inputs, bit 1: bottom input

 public:
  explicit SemiConductorGridTile(vi2d pos = vi2d(0, 0),
                        Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;

  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::SemiConductor; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

/**
 * @class ButtonGridTile
 * @brief Momentary signal source activated by user interaction.
 */
class ButtonGridTile : public LogicTile {
 public:
  explicit ButtonGridTile(vi2d pos = vi2d(0, 0),
                 Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;

  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Button; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

/**
 * @class InverterGridTile
 * @brief Inverts incoming signals.
 */
class InverterGridTile : public LogicTile {
 public:
  explicit InverterGridTile(vi2d pos = vi2d(0, 0),
                   Direction facing = Direction::Top);
  std::vector<SignalEvent> Init() override;
  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Inverter; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

/**
 * @class CrossingGridTile
 * @brief Allows signals to cross without interference.
 */
class CrossingGridTile : public LogicTile {
 public:
  explicit CrossingGridTile(vi2d pos = vi2d(0, 0),
                  Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Crossing; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

}  // namespace ElecSim
