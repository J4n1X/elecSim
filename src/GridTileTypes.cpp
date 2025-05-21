#include "GridTileTypes.h"

#include <cmath>

// --- WireGridTile Implementation ---

WireGridTile::WireGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::WHITE, olc::DARK_YELLOW) {
  // Can receive from all directions except facing direction
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    canReceive[i] = (static_cast<Direction>(i) != facing);
    canOutput[i] = (static_cast<Direction>(i) == facing);
    inputStates[i] = false;
  }
}

std::vector<SignalEvent> WireGridTile::ProcessSignal(
    const SignalEvent& signal) {
  // Update the input state for this direction
  inputStates[static_cast<int>(FlipDirection(signal.fromDirection))] =
      signal.isActive;

  // Check if any input is active
  bool shouldBeActive = false;
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    if (canReceive[i] && inputStates[i]) {
      shouldBeActive = true;
      break;
    }
  }

  // Only propagate state change if our activation state changes
  if (shouldBeActive != activated) {
    activated = shouldBeActive;
    return {SignalEvent(pos, facing, activated)};
  }

  return {};
}

void WireGridTile::ResetActivation() {
  GridTile::ResetActivation();
  // Reset all input states
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    inputStates[i] = false;
  }
}

// --- JunctionGridTile Implementation ---

JunctionGridTile::JunctionGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::GREY, olc::YELLOW) {
  // Can receive from one direction and output to all others
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    Direction dir = static_cast<Direction>(i);
    canReceive[i] = (dir == FlipDirection(facing));
    canOutput[i] = (dir != FlipDirection(facing));
  }
}

std::vector<SignalEvent> JunctionGridTile::ProcessSignal(
    const SignalEvent& signal) {
  if (signal.isActive == activated) return {};  // Prevent feedback loops
  if (!signal.isActive) {
    activated = false;
    std::vector<SignalEvent> events;
    for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
      if (canOutput[i]) {
        events.push_back(SignalEvent(pos, static_cast<Direction>(i), false));
      }
    }
    return events;
  }

  activated = true;
  std::vector<SignalEvent> events;
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    if (canOutput[i] && static_cast<Direction>(i) != FlipDirection(facing)) {
      events.push_back(SignalEvent(pos, static_cast<Direction>(i), true));
    }
  }
  return events;
}

// --- EmitterGridTile Implementation ---

EmitterGridTile::EmitterGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::DARK_CYAN, olc::CYAN),
      enabled(true),
      lastEmitTick(-EMIT_INTERVAL) {
  canOutput[static_cast<int>(facing)] = true;
  // Can receive from all directions except facing direction
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    canReceive[i] = (static_cast<Direction>(i) != facing);
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

SemiConductorGridTile::SemiConductorGridTile(olc::vi2d pos, Direction facing,
                                             float size)
    : GridTile(pos, facing, size, false, olc::DARK_GREEN, olc::GREEN),
      internalState(0) {
  // Can receive from sides and bottom relative to facing
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    Direction dir = static_cast<Direction>(i);
    Direction rotated = RotateDirection(dir, facing);
    canReceive[i] =
        (rotated == Direction::Left || rotated == Direction::Right ||
         rotated == Direction::Bottom);
    canOutput[i] = (dir == facing);
  }
}

std::vector<SignalEvent> SemiConductorGridTile::ProcessSignal(
    const SignalEvent& signal) {
  // Convert signal direction to facing-relative direction
  // Here's the sides and their flags:
  // 0b0001 = Left
  // 0b0010 = Right
  // 0b0100 = Bottom
  Direction relativeDir =
      RotateDirection(FlipDirection(signal.fromDirection), facing);

  // Update internal state based on inputs
  if (relativeDir == Direction::Left) {
    // set the bit to the activation given
    internalState =
        internalState & (0b110) |
        (0b001 & static_cast<int>(signal.isActive) << 0);  // Set side input bit
  }
  if (relativeDir == Direction::Right) {
    internalState = internalState & (0b101) |
                    (0b010 & static_cast<int>(signal.isActive) << 1);
  }
  if (relativeDir == Direction::Bottom) {
    internalState = internalState & (0b011) |
                    (0b100 & static_cast<int>(signal.isActive) << 2);
  }

  // Check if both side and bottom are active
  if ((internalState & 0b11) && (internalState & 0b100)) {
    if (activated) return {};  // Prevent feedback loops
    activated = true;
    return {SignalEvent(pos, facing, true)};
  } else if (activated) {
    activated = false;
    return {SignalEvent(pos, facing, false)};
  }

  return {};
}

void SemiConductorGridTile::ResetActivation() {
  activated = defaultActivation;
  internalState = 0;
}

std::vector<SignalEvent> SemiConductorGridTile::Interact() {
  internalState ^= 1;  // Toggle side input bit
  return {};
}

// --- ButtonGridTile Implementation ---

ButtonGridTile::ButtonGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::DARK_RED, olc::RED) {
  canOutput[static_cast<int>(facing)] = true;
}

std::vector<SignalEvent> ButtonGridTile::ProcessSignal(
    [[maybe_unused]] const SignalEvent& signal) {
  return {SignalEvent(pos, facing, activated)};
}

std::vector<SignalEvent> ButtonGridTile::Interact() {
  activated = !activated;
  return {SignalEvent(pos, facing, activated)};
}
