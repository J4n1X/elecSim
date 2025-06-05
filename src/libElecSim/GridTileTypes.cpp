#include "GridTileTypes.h"

#include <cmath>
#include <ranges>

namespace ElecSim {

// --- WireGridTile Implementation ---

WireGridTile::WireGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::WHITE, olc::DARK_YELLOW) {
  // Can receive from all directions except facing direction
  for (auto& dir : AllDirections) {
    canReceive[dir] = (dir != facing);
    canOutput[dir] = (dir == facing);
    inputStates[dir] = false;
  }
}

std::vector<SignalEvent> WireGridTile::ProcessSignal(
    const SignalEvent& signal) {
  // Restore previous logic: flip direction for inputStates update
  inputStates[signal.fromDirection] = signal.isActive;

  // Check if any input is active (from a direction we can receive from)
  bool shouldBeActive = false;
  for (auto& dir : AllDirections) {
    if (canReceive[dir] && inputStates[dir]) {
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

std::vector<SignalEvent> WireGridTile::PreprocessSignal(
    const std::vector<SignalEvent> incomingSignals) {
  for (auto& signal : incomingSignals) {
    if (signal.isActive) {
      return {SignalEvent(pos, facing, true)};
    }
  }
  return {SignalEvent(pos, facing, false)};
}

// --- JunctionGridTile Implementation ---

JunctionGridTile::JunctionGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::GREY, olc::YELLOW) {
  auto inputDir = FlipDirection(facing);
  // Can receive from one direction and output to all others
  std::fill(canOutput.begin(), canOutput.end(), true);
  canOutput[inputDir] = false;
  canReceive[inputDir] = true;
}

std::vector<SignalEvent> JunctionGridTile::ProcessSignal(
    const SignalEvent& signal) {
  if (signal.isActive == activated) return {};  // Prevent feedback loops
  if (!signal.isActive) {
    activated = false;
    std::vector<SignalEvent> events;
    for (auto& dir : AllDirections) {
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
    const std::vector<SignalEvent> incomingSignals) {
  std::vector<SignalEvent> events;
  for (const auto& dir : AllDirections) {
    if (canOutput[dir]) {
      events.push_back(SignalEvent(pos, dir, false));
    }
  }

  for (auto& signal : incomingSignals) {
    Direction toDir = FlipDirection(facing);
    if (signal.isActive && canReceive[toDir]) {
      for (auto& signal : events) {
        signal.isActive = true;
      }
      return events;
    }
  }
  return events;
}

// --- EmitterGridTile Implementation ---

EmitterGridTile::EmitterGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::DARK_CYAN, olc::CYAN),
      enabled(true),
      lastEmitTick(-EMIT_INTERVAL) {
  canOutput[facing] = true;
  // Can receive from all directions except facing direction
  for (auto& dir : AllDirections) {
    canReceive[dir] = (dir != facing);
  }
}

std::vector<SignalEvent> EmitterGridTile::ProcessSignal(
    [[maybe_unused]] const SignalEvent& signal) {
  return {SignalEvent(pos, facing, activated)};
}

std::vector<SignalEvent> EmitterGridTile::PreprocessSignal(
    const std::vector<SignalEvent> incomingSignals) {
  (void)incomingSignals;
  throw std::runtime_error(
      "Tried to preprocess an EmitterGridTile, which is not deterministic.");
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
  canReceive.fill(true);
  canOutput[facing] = true;
  canReceive[facing] = false;
}

std::vector<SignalEvent> SemiConductorGridTile::ProcessSignal(
    const SignalEvent& signal) {
  // Restore previous logic: flip direction for inputStates update
  inputStates[signal.fromDirection] = signal.isActive;

  // Check if both side (left or right) and bottom are active
  bool sideActive = inputStates[RotateDirection(Direction::Left, facing)] ||
                    inputStates[RotateDirection(Direction::Right, facing)];
  bool bottomActive = inputStates[RotateDirection(Direction::Bottom, facing)];

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

std::vector<SignalEvent> SemiConductorGridTile::PreprocessSignal(
    const std::vector<SignalEvent> incomingSignals) {
  (void)incomingSignals;
  throw std::runtime_error(
      "Tried to preprocess a SemiConductorGridTile, which is not deterministic.");
}

// --- ButtonGridTile Implementation ---

ButtonGridTile::ButtonGridTile(olc::vi2d pos, Direction facing, float size)
    : GridTile(pos, facing, size, false, olc::DARK_RED, olc::RED) {
  canOutput[facing] = true;
}

std::vector<SignalEvent> ButtonGridTile::ProcessSignal(
    [[maybe_unused]] const SignalEvent& signal) {
  return {SignalEvent(pos, facing, activated)};
}

std::vector<SignalEvent> ButtonGridTile::PreprocessSignal(
    const std::vector<SignalEvent> incomingSignals) {
  (void)incomingSignals;
  throw std::runtime_error(
      "Tried to preprocess a ButtonGridTile, which is not deterministic.");
}

std::vector<SignalEvent> ButtonGridTile::Interact() {
  activated = !activated;
  return {SignalEvent(pos, facing, activated)};
}

// --- InverterGridTile Implementation ---

std::vector<SignalEvent> InverterGridTile::Init() {
  return {SignalEvent(pos, FlipDirection(facing), false)};
}

std::vector<SignalEvent> InverterGridTile::ProcessSignal(
    const SignalEvent& signal) {
  inputStates[signal.fromDirection] = signal.isActive;

  // Check if any input is active
  bool shouldBeActive = false;
  for (auto& dir : AllDirections) {
    if (canReceive[dir] && inputStates[dir]) {
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
  for (auto& dir : AllDirections) {
    canReceive[dir] = true;
    canOutput[dir] = true;
    inputStates[dir] = false;
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
  float lineThickness = screenSize / 10.0f;  // Adjust thickness as needed

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

std::vector<SignalEvent> CrossingGridTile::PreprocessSignal(
    const std::vector<SignalEvent> incomingSignals) {
  if(incomingSignals.size() > 2 ){
    throw std::runtime_error(
        "CrossingGridTile can only handle up to 2 incoming signals.");
  }
  std::vector<SignalEvent> events;
  for(const auto& signal : incomingSignals) {
    // For each incoming signal, we output it to the opposite direction
    Direction outputDir = FlipDirection(signal.fromDirection);
    events.push_back(SignalEvent(pos, outputDir, signal.isActive));
  }
  return events;
}

}  // namespace ElecSim
