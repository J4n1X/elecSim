#pragma once

namespace Engine {
namespace GameStates {

enum class GameState : unsigned char { BuildMode, SimulationMode, DialogMode };

// Unified event enum
enum class Event : unsigned char {
  // General game events
  ConsoleToggle,
  BuildModeToggle,
  Save,
  Load,
  Quit,
  
  // Camera events
  CameraZoomIn,
  CameraZoomOut,
  CameraPanStart,
  CameraPanEnd,
  CameraMoveUp,
  CameraMoveRight,
  CameraMoveDown,
  CameraMoveLeft,
  
  // Build mode events
  BuildModeTileSelect,
  BuildModeCopy,
  BuildModePaste,
  BuildModeCut,
  BuildModeDelete,
  BuildModeRotate,
  BuildModeSelect,
  BuildModeClearBuffer,
  
  // Simulation events
  SimModeTileInteract,
  SimModeTickSpeedUp,
  SimModeTickSpeedDown
};

inline constexpr const char* EventToString(Event event);

}  // namespace GameStates
}  // namespace Engine