/**
 * Example integration of ControlManager into the main game loop
 * This shows how to replace the existing input handling in main.cpp
 * with the centralized ControlManager approach.
 */

#include "engine/ControlManager.h"
#include "engine/GameStates.h"

// Example of how to integrate ControlManager into the Game class
class GameWithControlManager : public olc::PixelGameEngine {
private:
    Engine::ControlManager* controlManager = nullptr;
    bool engineRunning = true;
    bool paused = true;
    bool unsavedChanges = false;
    int selectedBrushIndex = 1;

public:
    bool OnUserCreate() override {
        // Initialize the control manager
        controlManager = new Engine::ControlManager(this);
        
        // ...existing initialization code...
        
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        // Process input using ControlManager
        if (auto event = controlManager->ProcessInput()) {
            HandleGameEvent(*event, fElapsedTime);
        }

        // ...existing update logic...
        
        return engineRunning;
    }

    void HandleGameEvent(Engine::GameStates::Event event, float deltaTime) {
        switch (event) {
            case Engine::GameStates::Event::Quit:
                engineRunning = false;
                break;

            case Engine::GameStates::Event::ConsoleToggle:
                ConsoleShow(olc::Key::F1, false);
                break;

            case Engine::GameStates::Event::Save:
                SaveGrid();
                break;

            case Engine::GameStates::Event::Load:
                if (unsavedChanges) {
                    // Show unsaved changes dialog
                    // unsavedChangesGui->Enable();
                } else {
                    LoadGrid();
                }
                break;

            case Engine::GameStates::Event::BuildModeToggle:
                if (paused) {
                    // Reset simulation when unpausing
                    // grid.ResetSimulation();
                }
                paused = !paused;
                break;

            // Camera events
            case Engine::GameStates::Event::CameraMoveUp:
                HandleCameraMovement(olc::vf2d(0.0f, 1.0f), deltaTime);
                break;

            case Engine::GameStates::Event::CameraMoveDown:
                HandleCameraMovement(olc::vf2d(0.0f, -1.0f), deltaTime);
                break;

            case Engine::GameStates::Event::CameraMoveLeft:
                HandleCameraMovement(olc::vf2d(1.0f, 0.0f), deltaTime);
                break;

            case Engine::GameStates::Event::CameraMoveRight:
                HandleCameraMovement(olc::vf2d(-1.0f, 0.0f), deltaTime);
                break;

            case Engine::GameStates::Event::CameraZoomIn:
                HandleCameraZoom(1.0f, deltaTime);
                break;

            case Engine::GameStates::Event::CameraZoomOut:
                HandleCameraZoom(-1.0f, deltaTime);
                break;

            case Engine::GameStates::Event::CameraPanStart:
                // Camera panning is handled internally by ControlManager
                HandleCameraPanning();
                break;

            // Build mode events
            case Engine::GameStates::Event::BuildModeTileSelect:
                selectedBrushIndex = controlManager->GetLastSelectedNumberKey();
                CreateBrushTile();
                break;

            case Engine::GameStates::Event::BuildModeRotate:
                RotateBufferTiles();
                break;

            case Engine::GameStates::Event::BuildModeCopy:
                // Copy selected tiles
                CopyTiles(/* selection area */);
                break;

            case Engine::GameStates::Event::BuildModePaste:
                // Paste tiles at current position
                PasteTiles(/* paste position */);
                break;

            case Engine::GameStates::Event::BuildModeCut:
                // Delete selected tiles or clear buffer
                if (controlManager->IsRightMouseHeld()) {
                    // Delete tile at mouse position
                    DeleteTileAtMouse();
                } else {
                    // Clear buffer (P key)
                    ClearBuffer();
                }
                break;

            case Engine::GameStates::Event::BuildModeSelect:
                // Handle selection start/end
                HandleSelectionMode();
                break;

            // Simulation mode events
            case Engine::GameStates::Event::SimModeTileInteract:
                // Interact with tile at mouse position
                InteractWithTileAtMouse();
                break;

            case Engine::GameStates::Event::SimModeTickSpeedUp:
                // updateInterval += 0.025f;
                break;

            case Engine::GameStates::Event::SimModeTickSpeedDown:
                // updateInterval = std::max(0.0f, updateInterval - 0.025f);
                break;
        }
    }

private:
    void HandleCameraMovement(const olc::vf2d& direction, float deltaTime) {
        // float velocity = 30.f * renderScale * deltaTime;
        // grid.SetRenderOffset(grid.GetRenderOffset() + direction * velocity);
    }

    void HandleCameraZoom(float zoomDirection, float deltaTime) {
        // float newScale = std::clamp(
        //     renderScale + (zoomDirection * renderScale * 0.1f),
        //     minRenderScale, maxRenderScale);
        // grid.SetRenderScale(newScale);
    }

    void HandleCameraPanning() {
        if (controlManager->IsPanning()) {
            auto currentMousePos = controlManager->GetMousePosition();
            auto panDelta = currentMousePos - controlManager->GetPanStartPos();
            // grid.SetRenderOffset(grid.GetRenderOffset() - panDelta / renderScale);
        }
    }

    void CreateBrushTile() {
        // Implementation based on selectedBrushIndex
    }

    void RotateBufferTiles() {
        // Implementation for rotating tiles in buffer
    }

    void CopyTiles(/* parameters */) {
        // Implementation for copying tiles
    }

    void PasteTiles(/* parameters */) {
        // Implementation for pasting tiles
    }

    void ClearBuffer() {
        // Implementation for clearing tile buffer
    }

    void DeleteTileAtMouse() {
        // Implementation for deleting tile at mouse position
    }

    void HandleSelectionMode() {
        // Implementation for selection mode
    }

    void InteractWithTileAtMouse() {
        // Implementation for tile interaction
    }

    void SaveGrid() {
        // Implementation for saving
    }

    void LoadGrid() {
        // Implementation for loading
    }

    ~GameWithControlManager() {
        delete controlManager;
    }
};

/**
 * Key benefits of using ControlManager:
 * 
 * 1. Separation of Concerns: Input handling is separated from game logic
 * 2. Maintainability: Easy to modify controls without touching game logic
 * 3. Testability: Can test input handling independently
 * 4. Consistency: All input handling follows the same pattern
 * 5. Extensibility: Easy to add new controls and events
 * 6. Readability: Game logic focuses on what to do, not how to detect input
 */
