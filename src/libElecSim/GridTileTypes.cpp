#include "GridTileTypes.h"

#include <ranges>

namespace ElecSim {

// --- WireGridTile Implementation ---

WireGridTile::WireGridTile(vi2d newPos, Direction newFacing)
    : DeterministicTile(newPos, newFacing, false) {
  // Can receive from all directions except facing direction
  for (const auto& dir : AllDirections) {
    canReceive[dir] = (dir != newFacing);
    canOutput[dir] = (dir == newFacing);
  }
  inputStates.fill(false);
}

std::vector<SignalEvent> WireGridTile::ProcessSignal(
    const SignalEvent& signal) {
  inputStates[signal.fromDirection] = signal.isActive;

  // Check if any input is active (from a direction we can receive from)
  bool shouldBeActive = std::ranges::any_of(
      AllDirections,
      [this](Direction dir) { return canReceive[dir] && inputStates[dir]; });

  // Only propagate state change if our activation state changes
  if (shouldBeActive != activated) {
    activated = shouldBeActive;
    return {SignalEvent(pos, facing, activated)};
  }

  return {};
}

std::vector<SignalEvent> WireGridTile::PreprocessSignal(
    const SignalEvent incomingSignal) {
  return {SignalEvent(pos, facing, incomingSignal.isActive)};
}

// --- JunctionGridTile Implementation ---

JunctionGridTile::JunctionGridTile(vi2d newPos, Direction newFacing)
    : DeterministicTile(newPos, newFacing, false) {
  auto inputDir = FlipDirection(newFacing);
  // Can receive from one direction and output to all others
  std::fill(canOutput.begin(), canOutput.end(), true);
  canOutput[inputDir] = false;
  canReceive[inputDir] = true;
}

std::vector<SignalEvent> JunctionGridTile::ProcessSignal(
    const SignalEvent& signal) {
  if (signal.isActive == activated) [[unlikely]]
    return {};  // Prevent feedback loops
  if (!signal.isActive) {
    activated = false;
    std::vector<SignalEvent> events;
    for (const auto& dir : AllDirections) {
      if (canOutput[dir]) {
        events.push_back(SignalEvent(pos, dir, false));
      }
    }
    return events;
  }

  activated = true;
  std::vector<SignalEvent> events;
  for (auto& dir : AllDirections) {
    if (canOutput[dir] && dir != FlipDirection(facing)) {
      events.push_back(SignalEvent(pos, dir, true));
    }
  }
  return events;
}

std::vector<SignalEvent> JunctionGridTile::PreprocessSignal(
    const SignalEvent incomingSignal) {
  std::vector<SignalEvent> events;
  for (const auto& dir : AllDirections) {
    if (canOutput[dir]) [[likely]] {
      events.push_back(SignalEvent(pos, dir, incomingSignal.isActive));
    }
  }
  return events;
}

// --- EmitterGridTile Implementation ---

EmitterGridTile::EmitterGridTile(vi2d newPos, Direction newFacing)
    : LogicTile(newPos, newFacing, false),
      enabled(true),
      lastEmitTick(-EMIT_INTERVAL) {
  canOutput[newFacing] = true;
  // Can receive from all directions except facing direction
  for (auto& dir : AllDirections) {
    canReceive[dir] = (dir != newFacing);
  }
}

std::vector<SignalEvent> EmitterGridTile::ProcessSignal(
    [[maybe_unused]] const SignalEvent& signal) {
  return {SignalEvent(pos, facing, activated)};
}

std::vector<SignalEvent> EmitterGridTile::Interact() {
  enabled = !enabled;
  if (!enabled) {
    activated = false;
    return {SignalEvent(pos, facing, false)};
  }
  return {};
}

void EmitterGridTile::ResetActivation() {
  activated = defaultActivation;
  enabled = true;
  lastEmitTick = -EMIT_INTERVAL;  // Reset timing
}

bool EmitterGridTile::ShouldEmit(int currentTick) const {
  return enabled && (currentTick - lastEmitTick >= EMIT_INTERVAL);
}

// --- SemiConductorGridTile Implementation ---

SemiConductorGridTile::SemiConductorGridTile(vi2d newPos, Direction newFacing)
    : LogicTile(newPos, newFacing, false) {
  // Only allow input from world directions that, relative to facing, are local
  // Left, Right, or Bottom
  canReceive.fill(true);
  canOutput[newFacing] = true;
  canReceive[newFacing] = false;
  inputStates.fill(false);
}

std::vector<SignalEvent> SemiConductorGridTile::ProcessSignal(
    const SignalEvent& signal) {
  // Update input states (indexed by world coordinates)
  inputStates[signal.fromDirection] = signal.isActive;

  // Check if both side (left or right) and bottom are active
  // Convert tile-relative directions to world coordinates to check inputStates
  Direction worldLeft = TileToWorldDirection(Direction::Left);
  Direction worldRight = TileToWorldDirection(Direction::Right);
  Direction worldBottom = TileToWorldDirection(Direction::Bottom);

  bool sideActive = inputStates[worldLeft] || inputStates[worldRight];
  bool bottomActive = inputStates[worldBottom];

  if (sideActive && bottomActive) {
    if (activated) return {};  // Prevent feedback loops
    activated = true;
    return {SignalEvent(pos, facing, true)};
  } else if (activated) {
    activated = false;
    return {SignalEvent(pos, facing, false)};
  }
  return {};
}

// --- ButtonGridTile Implementation ---

ButtonGridTile::ButtonGridTile(vi2d newPos, Direction newFacing)
    : LogicTile(newPos, newFacing, false) {
  canOutput[newFacing] = true;
}

std::vector<SignalEvent> ButtonGridTile::ProcessSignal(
    [[maybe_unused]] const SignalEvent& signal) {
  return {SignalEvent(pos, facing, activated)};
}

std::vector<SignalEvent> ButtonGridTile::Interact() {
  activated = !activated;
  return {SignalEvent(pos, facing, activated)};
}

// --- InverterGridTile Implementation ---

InverterGridTile::InverterGridTile(vi2d newPos, Direction newFacing)
    : LogicTile(newPos, newFacing, false) {
  for (const auto& dir : AllDirections) {
    canReceive[dir] = (dir != newFacing);
    canOutput[dir] = (dir == newFacing);
    inputStates[dir] = false;
  }
}

std::vector<SignalEvent> InverterGridTile::Init() {
  return {SignalEvent(pos, FlipDirection(facing), false)};
}

std::vector<SignalEvent> InverterGridTile::ProcessSignal(
    const SignalEvent& signal) {
  inputStates[signal.fromDirection] = signal.isActive;

  // Check if any input is active
  bool shouldBeActive = std::ranges::any_of(AllDirections, [&](Direction dir) {
    return canReceive[dir] && inputStates[dir];
  });

  // Invert the logic for output
  bool inverted = !shouldBeActive;
  if (inverted != activated) {
    activated = inverted;
    return {SignalEvent(pos, facing, activated)};
  }
  return {};
}

// --- CrossingGridTile Implementation ---

CrossingGridTile::CrossingGridTile(vi2d newPos, Direction newFacing)
    : LogicTile(newPos, newFacing, false) {
  // Allow receiving and outputting from all directions
  canReceive.fill(true);
  canOutput.fill(true);
  inputStates.fill(false);
}

std::vector<SignalEvent> CrossingGridTile::ProcessSignal(
    const SignalEvent& signal) {
  // Update the input state for the direction the signal came from
  Direction inputDir = signal.fromDirection;
  inputStates[inputDir] = signal.isActive;

  // In a crossing, we want to output signals to the opposite side
  Direction outputDir = FlipDirection(inputDir);

  // Return a signal event to the opposite direction
  return {SignalEvent(pos, outputDir, signal.isActive)};
}

// --- Clone Implementations ---
// I know, this is tedious but really, this is the best way to ensure a clone
// ends up in the valid state.

std::unique_ptr<GridTile> WireGridTile::Clone() const {
  auto clone = std::make_unique<WireGridTile>(*this);
  return clone;
}

std::unique_ptr<GridTile> JunctionGridTile::Clone() const {
  auto clone = std::make_unique<JunctionGridTile>(*this);
  return clone;
}

std::unique_ptr<GridTile> EmitterGridTile::Clone() const {
  auto clone = std::make_unique<EmitterGridTile>(*this);
  // Copy EmitterGridTile-specific state
  clone->enabled = this->enabled;
  clone->lastEmitTick = this->lastEmitTick;
  return clone;
}

std::unique_ptr<GridTile> SemiConductorGridTile::Clone() const {
  auto clone = std::make_unique<SemiConductorGridTile>(*this);
  return clone;
}

std::unique_ptr<GridTile> ButtonGridTile::Clone() const {
  auto clone = std::make_unique<ButtonGridTile>(*this);
  return clone;
}

std::unique_ptr<GridTile> InverterGridTile::Clone() const {
  auto clone = std::make_unique<InverterGridTile>(*this);
  return clone;
}

std::unique_ptr<GridTile> CrossingGridTile::Clone() const {
  auto clone = std::make_unique<CrossingGridTile>(*this);
  return clone;
}
}  // namespace ElecSim
