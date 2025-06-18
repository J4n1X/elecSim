#pragma once

#include "GridTile.h"

namespace ElecSim {

// Each derived tile class has its own responsibilities:

// Wire: Basic signal conductor, propagates signals in one direction
class WireGridTile : public DeterministicTile {
 public:
  explicit WireGridTile(vi2d pos = vi2d(0, 0),
               Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> PreprocessSignal(const SignalEvent incomingSignal) override;
  constexpr const char* TileTypeName() const override { return "Wire"; }
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Wire; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

// Junction: Multi-directional signal splitter
class JunctionGridTile : public DeterministicTile {
 public:
  explicit JunctionGridTile(vi2d pos = vi2d(0, 0),
                   Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> PreprocessSignal(const SignalEvent incomingSignal) override;
  constexpr const char* TileTypeName() const override { return "Junction"; }
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Junction; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

// Emitter: Signal source that can be toggled
class EmitterGridTile : public LogicTile {
 protected:
  bool enabled;
  static constexpr int EMIT_INTERVAL = 3;  // Emit every 3 ticks
  int lastEmitTick;                        // Last tick when signal was emitted

 public:
  explicit EmitterGridTile(vi2d pos = vi2d(0, 0),
                  Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;
  void ResetActivation() override;
  bool ShouldEmit(int currentTick) const;

  constexpr const char* TileTypeName() const override { return "Emitter"; }
  bool IsEmitter() const override { return true; }
  TileType GetTileType() const override { return TileType::Emitter; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

// SemiConductor: Logic gate that requires multiple inputs
class SemiConductorGridTile : public LogicTile {
 protected:
  int internalState;  // bit 0: side inputs, bit 1: bottom input

 public:
  explicit SemiConductorGridTile(vi2d pos = vi2d(0, 0),
                        Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;

  constexpr const char* TileTypeName() const override { return "Semiconductor"; }
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::SemiConductor; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

// Button: Momentary signal source
class ButtonGridTile : public LogicTile {
 public:
  explicit ButtonGridTile(vi2d pos = vi2d(0, 0),
                 Direction facing = Direction::Top);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;

  constexpr const char* TileTypeName() const override { return "Button"; }
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Button; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

// --- InverterGridTile: Inverts the signal from the base GridTile ---

class InverterGridTile : public LogicTile {
 public:
  explicit InverterGridTile(vi2d pos = vi2d(0, 0),
                   Direction facing = Direction::Top);
  std::vector<SignalEvent> Init() override;
  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  constexpr const char* TileTypeName() const override { return "Inverter"; }
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Inverter; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

// Crossing: Allows signals to cross without interference
class CrossingGridTile : public LogicTile {
 public:
  explicit CrossingGridTile(vi2d pos = vi2d(0, 0),
                  Direction facing = Direction::Top);

  //void Draw(PixelGameEngine* renderer, vf2d screenPos,
  //          float screenSize, int alpha = 255) override;
  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  
  constexpr const char* TileTypeName() const override { return "Crossing"; }
  bool IsEmitter() const override { return false; }
  TileType GetTileType() const override { return TileType::Crossing; }
  
  [[nodiscard]] std::unique_ptr<GridTile> Clone() const override;
};

}  // namespace ElecSim
