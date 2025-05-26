#include "GridTileTypes.h"

#include <cmath>

namespace ElecSim {

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
  // Restore previous logic: flip direction for inputStates update
  inputStates[static_cast<int>(FlipDirection(signal.fromDirection))] =
      signal.isActive;

  // Check if any input is active (from a direction we can receive from)
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

// --- JunctionGridTile Implementation ---

JunctionGridTile::JunctionGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::GREY, olc::YELLOW) {
  auto inputDir = static_cast<int>(FlipDirection(facing));
  // Can receive from one direction and output to all others
  std::fill_n(canOutput, static_cast<int>(Direction::Count), true);
  canOutput[inputDir] = false;
  canReceive[inputDir] = true;
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
    : GridTile(pos, facing, size, false, olc::DARK_GREEN, olc::GREEN) {
  // Only allow input from world directions that, relative to facing, are local
  // Left, Right, or Bottom
  std::fill_n(canReceive, static_cast<int>(Direction::Count), true);
  canOutput[static_cast<int>(facing)] = true;
  canReceive[static_cast<int>(facing)] = false;
}

std::vector<SignalEvent> SemiConductorGridTile::ProcessSignal(
    const SignalEvent& signal) {
  // Restore previous logic: flip direction for inputStates update
  inputStates[static_cast<int>(FlipDirection(signal.fromDirection))] =
      signal.isActive;

  // Check if both side (left or right) and bottom are active
  bool sideActive =
      inputStates[static_cast<int>(RotateDirection(Direction::Left, facing))] ||
      inputStates[static_cast<int>(RotateDirection(Direction::Right, facing))];
  bool bottomActive =
      inputStates[static_cast<int>(RotateDirection(Direction::Bottom, facing))];

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

void SemiConductorGridTile::ResetActivation() {
  activated = defaultActivation;
  // inputStates is reset in base class
}

std::vector<SignalEvent> SemiConductorGridTile::Interact() {
  // Toggle input for a specific direction (default: left) for demonstration
  int targetIdx = -1;
  Direction targetDirection = Direction::Left;  // Change this to generalize
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    if (RotateDirection(static_cast<Direction>(i), facing) == targetDirection) {
      targetIdx = i;
      break;
    }
  }
  if (targetIdx != -1) inputStates[targetIdx] = !inputStates[targetIdx];
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

// --- InverterGridTile Implementation ---

std::vector<SignalEvent> InverterGridTile::ProcessSignal(
    const SignalEvent& signal) {
  // Restore previous logic: flip direction for inputStates update
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

  // Invert the logic for output
  bool inverted = !shouldBeActive;
  if (inverted != activated) {
    activated = inverted;
    return {SignalEvent(pos, facing, activated)};
  }
  return {};
}

// --- CrossingGridTile Implementation ---

CrossingGridTile::CrossingGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::DARK_BLUE, olc::BLUE) {
  // Allow receiving and outputting from all directions
  for (int i = 0; i < static_cast<int>(Direction::Count); i++) {
    canReceive[i] = true;
    canOutput[i] = true;
    inputStates[i] = false;
  }
}

void CrossingGridTile::Draw(olc::PixelGameEngine* renderer, olc::vf2d screenPos,
                          float screenSize, int alpha) {
  // Get colors based on activation state and alpha
  olc::Pixel activeColor = this->activeColor;
  olc::Pixel inactiveColor = this->inactiveColor;
  activeColor.a = alpha;
  inactiveColor.a = alpha;
  
  // Draw the base square
  renderer->FillRectDecal(screenPos, olc::vi2d(screenSize, screenSize),
                         activated ? activeColor : inactiveColor);
  
  // Draw crossing lines that touch the edges
  float lineThickness = screenSize / 10.0f; // Adjust thickness as needed
  
  // Horizontal line - top left to bottom right
  renderer->FillRectDecal(
      olc::vf2d(screenPos.x, screenPos.y + (screenSize - lineThickness) / 2),
      olc::vf2d(screenSize, lineThickness),
      activated ? inactiveColor : activeColor);
  
  // Vertical line - top to bottom
  renderer->FillRectDecal(
      olc::vf2d(screenPos.x + (screenSize - lineThickness) / 2, screenPos.y),
      olc::vf2d(lineThickness, screenSize),
      activated ? inactiveColor : activeColor);
}

std::vector<SignalEvent> CrossingGridTile::ProcessSignal(const SignalEvent& signal) {
  // Update the input state for the direction the signal came from
  Direction inputDir = FlipDirection(signal.fromDirection);
  inputStates[static_cast<int>(inputDir)] = signal.isActive;
  
  // In a crossing, we want to output signals to the opposite side
  Direction outputDir = FlipDirection(inputDir);
  
  // Return a signal event to the opposite direction
  return {SignalEvent(pos, outputDir, signal.isActive)};
}

}  // namespace ElecSim
