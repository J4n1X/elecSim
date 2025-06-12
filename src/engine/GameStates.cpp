#include "GameStates.h"

namespace Engine {
namespace GameStates {
constexpr const char* EventToString(Event event) {
  switch (event) {
    case Event::ConsoleToggle:
      return "ConsoleToggle";
    case Event::BuildModeToggle:
      return "BuildModeToggle";
    case Event::Save:
      return "Save";
    case Event::Load:
      return "Load";
    case Event::Quit:
      return "Quit";
    case Event::CameraZoomIn:
      return "CameraZoomIn";
    case Event::CameraZoomOut:
      return "CameraZoomOut";
    case Event::CameraPan:
      return "CameraPan";
    case Event::CameraMoveUp:
      return "CameraMoveUp";
    case Event::CameraMoveRight:
      return "CameraMoveRight";
    case Event::CameraMoveDown:
      return "CameraMoveDown";
    case Event::CameraMoveLeft:
      return "CameraMoveLeft";
    case Event::BuildModeTileSelect:
      return "BuildModeTileSelect";
    case Event::BuildModeCopy:
      return "BuildModeCopy";
    case Event::BuildModePaste:
      return "BuildModePaste";
    case Event::BuildModeCut:
      return "BuildModeCut";
    case Event::BuildModeDelete:
      return "BuildModeDelete";
    case Event::BuildModeRotate:
      return "BuildModeRotate";
    case Event::BuildModeSelect:
      return "BuildModeSelect";
    case Event::BuildModeClearBuffer:
      return "BuildModeClearBuffer";
    case Event::SimModeTileInteract:
      return "SimModeTileInteract";
    case Event::SimModeTickSpeedUp:
      return "SimModeTickSpeedUp";
    case Event::SimModeTickSpeedDown:
      return "SimModeTickSpeedDown";
    default:
      return "UnknownEvent";
  }
}
}  // namespace GameStates
}  // namespace Engine
