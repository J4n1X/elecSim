#pragma once

#include "GridTile.h"

namespace ElecSim {

// Each derived tile class has its own responsibilities:

// Wire: Basic signal conductor, propagates signals in one direction
class WireGridTile : public DeterministicTile {
 public:
  explicit WireGridTile(olc::vi2d pos = olc::vi2d(0, 0),
               Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> PreprocessSignal(const SignalEvent incomingSignal) override;
  std::string_view TileTypeName() const override { return "Wire"; }
  bool IsEmitter() const override { return false; }
  int GetTileTypeId() const override { return 0; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<WireGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    return clone;
  }
};

// Junction: Multi-directional signal splitter
class JunctionGridTile : public DeterministicTile {
 public:
  explicit JunctionGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                   Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> PreprocessSignal(const SignalEvent incomingSignal) override;
  std::string_view TileTypeName() const override { return "Junction"; }
  bool IsEmitter() const override { return false; }
  int GetTileTypeId() const override { return 1; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<JunctionGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    return clone;
  }
};

// Emitter: Signal source that can be toggled
class EmitterGridTile : public LogicTile {
 private:
  bool enabled;
  static constexpr int EMIT_INTERVAL = 3;  // Emit every 3 ticks
  int lastEmitTick;                        // Last tick when signal was emitted

 public:
  explicit EmitterGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                  Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;
  void ResetActivation() override;
  bool ShouldEmit(int currentTick) const;

  std::string_view TileTypeName() const override { return "Emitter"; }
  bool IsEmitter() const override { return true; }
  int GetTileTypeId() const override { return 2; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<EmitterGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    
    // Copy additional internal state
    auto* typedClone = static_cast<EmitterGridTile*>(clone.get());
    typedClone->enabled = this->enabled;
    typedClone->lastEmitTick = this->lastEmitTick;
    
    return clone;
  }
};

// SemiConductor: Logic gate that requires multiple inputs
class SemiConductorGridTile : public LogicTile {
 private:
  int internalState;  // bit 0: side inputs, bit 1: bottom input

 public:
  explicit SemiConductorGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                        Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;

  std::string_view TileTypeName() const override { return "Semiconductor"; }
  bool IsEmitter() const override { return false; }
  int GetTileTypeId() const override { return 3; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<SemiConductorGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    
    // Copy additional internal state
    auto* typedClone = static_cast<SemiConductorGridTile*>(clone.get());
    typedClone->internalState = this->internalState;
    
    return clone;
  }
};

// Button: Momentary signal source
class ButtonGridTile : public LogicTile {
 public:
  explicit ButtonGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                 Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;

  std::string_view TileTypeName() const override { return "Button"; }
  bool IsEmitter() const override { return false; }
  int GetTileTypeId() const override { return 4; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<ButtonGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    return clone;
  }
};

// --- InverterGridTile: Inverts the signal from the base GridTile ---

class InverterGridTile : public LogicTile {
 public:
  explicit InverterGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                   Direction facing = Direction::Top, float size = .10f)
      : LogicTile(pos, facing, size, false, olc::DARK_MAGENTA, olc::MAGENTA) {
    for (const auto& dir : AllDirections) {
      canReceive[dir] = (dir != facing);
      canOutput[dir] = (dir == facing);
      inputStates[dir] = false;
    }
  }
  std::vector<SignalEvent> Init() override;
  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::string_view TileTypeName() const override { return "Inverter"; }
  bool IsEmitter() const override { return false; }
  int GetTileTypeId() const override { return 5; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<InverterGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    return clone;
  }
};

// Crossing: Allows signals to cross without interference
class CrossingGridTile : public LogicTile {
 public:
  explicit CrossingGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                  Direction facing = Direction::Top, float size = 1.0f);

  void Draw(olc::PixelGameEngine* renderer, olc::vf2d screenPos,
            float screenSize, int alpha = 255) override;
  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  
  std::string_view TileTypeName() const override { return "Crossing"; }
  bool IsEmitter() const override { return false; }
  int GetTileTypeId() const override { return 6; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<CrossingGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    
    // Copy the input states
    for (const auto& dir : AllDirections) {
      clone->inputStates[dir] = this->inputStates[dir];
    }
    
    return clone;
  }
};

}  // namespace ElecSim
