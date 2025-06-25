#pragma once

#include "olcPixelGameEngine.h"
#include "extensions/olcPGEX_QuickGUI.h"

class MessageBoxGui {
public:
  enum Result {
    Yes,
    No,
    Cancel,
    Nothing
  };

  MessageBoxGui(olc::PixelGameEngine* pge, std::string labelText, float scale, bool enabled);
  
  // No copying, only moving
  MessageBoxGui(const MessageBoxGui&) = delete;
  MessageBoxGui& operator=(const MessageBoxGui&) = delete;

  ~MessageBoxGui();
  
  Result Update();
  void Enable();
  void Disable();
  bool IsEnabled() {return enabled;}
  void SetRenderScale(float scale);
  void Draw();

private:
  bool enabled;
  float renderScale;
  std::string labelText;
  olc::PixelGameEngine* engine;
  // --- QuickGui Variables ---
  olc::QuickGUI::Manager guiManager;
  olc::QuickGUI::Label* unsavedChangesInfoLabel = nullptr;
  olc::QuickGUI::Button* yesButton = nullptr;
  olc::QuickGUI::Button* noButton = nullptr;
  olc::QuickGUI::Button* cancelButton = nullptr;
};
