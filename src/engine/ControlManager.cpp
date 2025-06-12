#include "ControlManager.h"

namespace Engine {

const std::vector<GameStates::Event>& ControlManager::ProcessInput() {
  // Clear the event buffer from previous frame
  eventBuffer.clear();
  
  if (!pge) return eventBuffer;
  
  // Don't process input if console is active
  if (IsConsoleActive()) return eventBuffer;
  
  // Collect all events that occurred this frame
  // Each function can push multiple events to eventBuffer
  
  // 1. General game controls (highest priority)
  CheckGeneralInputs();
  
  // 2. Camera controls
  CheckCameraInputs();
  
  // 3. Build mode specific controls
  CheckBuildModeInputs();
  
  // 4. Simulation mode specific controls
  CheckSimModeInputs();
  
  return eventBuffer;
}

bool ControlManager::IsConsoleActive() const {
  // Check if console is showing (based on main.cpp pattern)
  return pge->IsConsoleShowing();
}

void ControlManager::CheckGeneralInputs() {
  // Console toggle (F1)
  if (pge->GetKey(olc::Key::F1).bPressed) {
    eventBuffer.push_back(GameStates::Event::ConsoleToggle);
  }
  
  // Save (F2)
  if (pge->GetKey(olc::Key::F2).bPressed) {
    eventBuffer.push_back(GameStates::Event::Save);
  }
  
  // Load (F3)
  if (pge->GetKey(olc::Key::F3).bPressed) {
    eventBuffer.push_back(GameStates::Event::Load);
  }
  
  // Quit (Escape)
  if (pge->GetKey(olc::Key::ESCAPE).bPressed) {
    eventBuffer.push_back(GameStates::Event::Quit);
  }
  
  // Build/Simulation mode toggle (Space)
  if (pge->GetKey(olc::Key::SPACE).bPressed) {
    eventBuffer.push_back(GameStates::Event::BuildModeToggle);
  }
}

void ControlManager::CheckCameraInputs() {
  // Pan with middle mouse button
  if (pge->GetMouse(2).bPressed) {
    isPanning = true;
    panStartPos = pge->GetMousePos();
    eventBuffer.push_back(GameStates::Event::CameraPan);
  }
  
  if (pge->GetMouse(2).bHeld) {
    eventBuffer.push_back(GameStates::Event::CameraPan);
  }

  if (pge->GetMouse(2).bReleased) {
    isPanning = false;
  }
  
  // Camera movement with arrow keys - can have multiple directions simultaneously
  if (pge->GetKey(olc::Key::UP).bHeld || pge->GetKey(olc::Key::W).bHeld) {
    eventBuffer.push_back(GameStates::Event::CameraMoveUp);
  }
  
  if (pge->GetKey(olc::Key::DOWN).bHeld || pge->GetKey(olc::Key::S).bHeld) {
    eventBuffer.push_back(GameStates::Event::CameraMoveDown);
  }

  if (pge->GetKey(olc::Key::LEFT).bHeld || pge->GetKey(olc::Key::A).bHeld) {
    eventBuffer.push_back(GameStates::Event::CameraMoveLeft);
  }

  if (pge->GetKey(olc::Key::RIGHT).bHeld || pge->GetKey(olc::Key::D).bHeld) {
    eventBuffer.push_back(GameStates::Event::CameraMoveRight);
  }
  
  // Zoom with J/K keys (based on main.cpp pattern)
  if (int zoomDelta = pge->GetMouseWheel(); zoomDelta != 0) {
    if (zoomDelta > 0) {
        eventBuffer.push_back(GameStates::Event::CameraZoomIn);
    } else {
        eventBuffer.push_back(GameStates::Event::CameraZoomOut);
    }
  }

  if(pge->GetKey(olc::Key::C).bPressed){
    eventBuffer.push_back(GameStates::Event::CameraReset);
  }
}

void ControlManager::CheckBuildModeInputs() {
  // Number keys for tile selection (0-9) - can have multiple number keys pressed
  int numberKey = ParseNumberKey();
  if (numberKey >= 0) {
    lastSelectedNumberKey = numberKey;
    eventBuffer.push_back(GameStates::Event::BuildModeTileSelect);
  }
  
  // Rotation with R key
  if (pge->GetKey(olc::Key::R).bPressed) {
    eventBuffer.push_back(GameStates::Event::BuildModeRotate);
  }
  
  // Copy with Ctrl+C
  if (pge->GetKey(olc::Key::CTRL).bHeld && pge->GetKey(olc::Key::C).bPressed) {
    eventBuffer.push_back(GameStates::Event::BuildModeCopy);
  }

  // Paste with Ctrl+V or by pressing the left mouse button.
  if ((pge->GetKey(olc::Key::CTRL).bHeld && pge->GetKey(olc::Key::V).bPressed) || pge->GetMouse(0).bHeld) {
    eventBuffer.push_back(GameStates::Event::BuildModePaste);
  }
  
  // Cut with Ctrl+X
  if (pge->GetKey(olc::Key::CTRL).bHeld && pge->GetKey(olc::Key::X).bPressed) {
    eventBuffer.push_back(GameStates::Event::BuildModeCut);
  }

  // Delete with right mouse button
  if (pge->GetMouse(1).bHeld) {
    eventBuffer.push_back(GameStates::Event::BuildModeDelete);
  }
  
  // Selection with Ctrl (start/end)
  if (pge->GetKey(olc::Key::CTRL).bPressed || pge->GetKey(olc::Key::CTRL).bReleased) {
    eventBuffer.push_back(GameStates::Event::BuildModeSelect);
  }
  
  // Clear buffer with P key
  if (pge->GetKey(olc::Key::P).bPressed) {
    eventBuffer.push_back(GameStates::Event::BuildModeClearBuffer);
  }
}

void ControlManager::CheckSimModeInputs() {
  // Tile interaction with left mouse click (in simulation mode)
  if (pge->GetMouse(0).bPressed) {
    eventBuffer.push_back(GameStates::Event::SimModeTileInteract);
  }
  
  // Speed controls
  if (pge->GetKey(olc::Key::PERIOD).bPressed) {
    eventBuffer.push_back(GameStates::Event::SimModeTickSpeedUp);
  }
  
  if (pge->GetKey(olc::Key::COMMA).bPressed) {
    eventBuffer.push_back(GameStates::Event::SimModeTickSpeedDown);
  }
}

int ControlManager::ParseNumberKey() {
  // Check for number keys 0-9 (based on main.cpp pattern)
  for (int i = static_cast<int>(olc::Key::K0); i <= static_cast<int>(olc::Key::K9); i++) {
    olc::Key key = static_cast<olc::Key>(i);
    if (pge->GetKey(key).bPressed) {
      return i - static_cast<int>(olc::Key::K0);
    }
  }
  return -1; // No number key pressed
}

// Mouse button state helpers
bool ControlManager::IsLeftMousePressed() const {
  return pge ? pge->GetMouse(0).bPressed : false;
}

bool ControlManager::IsLeftMouseHeld() const {
  return pge ? pge->GetMouse(0).bHeld : false;
}

bool ControlManager::IsRightMousePressed() const {
  return pge ? pge->GetMouse(1).bPressed : false;
}

bool ControlManager::IsRightMouseHeld() const {
  return pge ? pge->GetMouse(1).bHeld : false;
}

bool ControlManager::IsMiddleMousePressed() const {
  return pge ? pge->GetMouse(2).bPressed : false;
}

bool ControlManager::IsMiddleMouseHeld() const {
  return pge ? pge->GetMouse(2).bHeld : false;
}

olc::vi2d ControlManager::GetMousePosition() const {
  return pge ? pge->GetMousePos() : olc::vi2d{0, 0};
}

}  // namespace Engine