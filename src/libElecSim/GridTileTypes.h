#pragma once

#include "GridTile.h"

namespace ElecSim {

// Each derived tile class has its own responsibilities:

// Wire: Basic signal conductor, propagates signals in one direction
class WireGridTile : public GridTile {
 public:
  WireGridTile(olc::vi2d pos = olc::vi2d(0, 0),
               Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::string_view TileTypeName() const override { return "Wire"; }
  bool IsEmitter() const override { return false; }
  int GetTileId() const override { return 0; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<WireGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    return clone;
  }
};

// Junction: Multi-directional signal splitter
class JunctionGridTile : public GridTile {
 public:
  JunctionGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                   Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::string_view TileTypeName() const override { return "Junction"; }
  bool IsEmitter() const override { return false; }
  int GetTileId() const override { return 1; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<JunctionGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    return clone;
  }
};

// Emitter: Signal source that can be toggled
class EmitterGridTile : public GridTile {
 private:
  bool enabled;
  static constexpr int EMIT_INTERVAL = 3;  // Emit every 3 ticks
  int lastEmitTick;                        // Last tick when signal was emitted

 public:
  EmitterGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                  Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;
  void ResetActivation() override;
  bool ShouldEmit(int currentTick) const;

  std::string_view TileTypeName() const override { return "Emitter"; }
  bool IsEmitter() const override { return true; }
  int GetTileId() const override { return 2; }
  
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
class SemiConductorGridTile : public GridTile {
 private:
  int internalState;  // bit 0: side inputs, bit 1: bottom input

 public:
  SemiConductorGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                        Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;
  void ResetActivation() override;

  std::string_view TileTypeName() const override { return "Semiconductor"; }
  bool IsEmitter() const override { return false; }
  int GetTileId() const override { return 3; }
  
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
class ButtonGridTile : public GridTile {
 public:
  ButtonGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                 Direction facing = Direction::Top, float size = 1.0f);

  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::vector<SignalEvent> Interact() override;

  std::string_view TileTypeName() const override { return "Button"; }
  bool IsEmitter() const override { return false; }
  int GetTileId() const override { return 4; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<ButtonGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    return clone;
  }
};

// --- InverterGridTile: Inverts the signal from the base GridTile ---

class InverterGridTile : public GridTile {
 public:
  InverterGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                   Direction facing = Direction::Top, float size = .10f)
      : GridTile(pos, facing, size, false, olc::DARK_MAGENTA, olc::MAGENTA) {
    for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
      canReceive[i] = (static_cast<Direction>(i) != facing);
      canOutput[i] = (static_cast<Direction>(i) == facing);
      inputStates[i] = false;
    }
  }
  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  std::string_view TileTypeName() const override { return "Inverter"; }
  bool IsEmitter() const override { return false; }
  int GetTileId() const override { return 5; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<InverterGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    return clone;
  }
};

// Crossing: Allows signals to cross without interference
class CrossingGridTile : public GridTile {
 public:
  CrossingGridTile(olc::vi2d pos = olc::vi2d(0, 0),
                  Direction facing = Direction::Top, float size = 1.0f);

  void Draw(olc::PixelGameEngine* renderer, olc::vf2d screenPos,
            float screenSize, int alpha = 255) override;
  std::vector<SignalEvent> ProcessSignal(const SignalEvent& signal) override;
  
  std::string_view TileTypeName() const override { return "Crossing"; }
  bool IsEmitter() const override { return false; }
  int GetTileId() const override { return 6; }
  
  std::unique_ptr<GridTile> Clone() const override {
    auto clone = std::make_unique<CrossingGridTile>(GetPos(), GetFacing(), GetSize());
    clone->SetActivation(GetActivation());
    clone->SetDefaultActivation(GetDefaultActivation());
    
    // Copy the input states
    for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
      clone->inputStates[i] = this->inputStates[i];
    }
    
    return clone;
  }
};

}  // namespace ElecSim
