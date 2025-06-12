#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "GameStates.h"
#include "olcPixelGameEngine.h"

namespace Engine {

  class ControlManager {
    private:
      olc::PixelGameEngine* pge = nullptr;
      bool isPanning = false;
      olc::vi2d panStartPos = {0, 0};
      int lastSelectedNumberKey = -1; // Last number key pressed (0-9)
      
      // Persistent vector to avoid repeated allocations
      mutable std::vector<GameStates::Event> eventBuffer;
      
    public:
      ControlManager(olc::PixelGameEngine* pge) : pge(pge) {}
      ~ControlManager() = default;

      bool IsConsoleActive() const;
      bool IsLeftMousePressed() const;
      bool IsLeftMouseHeld() const;
      bool IsRightMousePressed() const;
      bool IsRightMouseHeld() const;
      bool IsMiddleMousePressed() const;
      bool IsMiddleMouseHeld() const;
      olc::vi2d GetMousePosition() const;

      const std::vector<GameStates::Event>& ProcessInput();

      void CheckGeneralInputs();
    
      void CheckCameraInputs();
    
      void CheckBuildModeInputs();
    
      void CheckSimModeInputs();
    
      // Helper methods for game state
      bool IsPanning() const { return isPanning; }
      olc::vi2d GetPanStartPos() const { return panStartPos; }
      int GetLastSelectedNumberKey() const { return lastSelectedNumberKey; }
      
      int ParseNumberKey();
  };


}  // namespace Engine