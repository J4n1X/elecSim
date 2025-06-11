#include "olcPixelGameEngine.h"
#include "extensions/olcPGEX_QuickGUI.h"

#include "MessageBox.h"

MessageBoxGui::MessageBoxGui(olc::PixelGameEngine* pge, std::string labelText, float scale, bool enabled){
    engine = pge;
    this->labelText = labelText;
    renderScale = scale;
    this->enabled = enabled;
    
    guiManager = olc::QuickGUI::Manager(false);
    unsavedChangesInfoLabel = new olc::QuickGUI::Label(
        guiManager, labelText,
        {0.0f, 0.0f}, {renderScale*3.0f, renderScale});
    unsavedChangesInfoLabel->nAlign = olc::QuickGUI::Label::Alignment::Centre;
    yesButton = new olc::QuickGUI::Button(
        guiManager, "Yes", {0.0f, 1.0f * renderScale}, {renderScale , renderScale / 2.0f});
    noButton = new olc::QuickGUI::Button(
        guiManager, "No", {1.0f * renderScale, 1.0f * renderScale},
        {renderScale, renderScale / 2.0f});
    cancelButton = new olc::QuickGUI::Button(
        guiManager, "Cancel", {2.0f * renderScale, 1.0f * renderScale},
        {renderScale, renderScale / 2.0f});
  }

void MessageBoxGui::Enable(){
  enabled = true;
  unsavedChangesInfoLabel->Enable(true);
  yesButton->Enable(true);
  noButton->Enable(true);
  cancelButton->Enable(true);
}

void MessageBoxGui::Disable(){
  enabled = false;
  unsavedChangesInfoLabel->Enable(false);
  yesButton->Enable(false);
  noButton->Enable(false);
  cancelButton->Enable(false);
}

MessageBoxGui::~MessageBoxGui() {
    delete unsavedChangesInfoLabel;
    delete yesButton;
    delete noButton;
    delete cancelButton;
}
MessageBoxGui::Result MessageBoxGui::Update() {
    if(engine == nullptr)
      throw std::runtime_error("Engine pointer is null in UnsavedChangesGui");
    if(enabled){
      guiManager.Update(engine);
      if (yesButton->bPressed) {
        return Result::Yes;
      } else if (noButton->bPressed) {
        return Result::No;
      } else if (cancelButton->bPressed) {
        return Result::Cancel;
      }
    }
    return Result::Nothing;
}
void MessageBoxGui::SetRenderScale(float scale) {
    renderScale = scale;
    if (unsavedChangesInfoLabel) {
      unsavedChangesInfoLabel->vSize = {renderScale * 1.5f, renderScale * 1.5f};
    }
    if (yesButton) {
      yesButton->vSize = {renderScale, renderScale / 2.0f};
      yesButton->vPos = {0.0f, renderScale};
    }
    if (noButton) {
      noButton->vSize = {renderScale, renderScale / 2.0f};
      noButton->vPos = {renderScale, renderScale};
    }
    if (cancelButton) {
      cancelButton->vSize = {renderScale, renderScale / 2.0f};
      cancelButton->vPos = {2.0f * renderScale, renderScale};
    }
}

void MessageBoxGui::Draw() {
  const olc::Pixel backgroundColor = olc::Pixel(0, 0, 128, 128);
    if(engine == nullptr)
      throw std::runtime_error("Engine pointer is null in UnsavedChangesGui");
    if(!enabled) return;
    // Get center of screen position and then subtract half the size of the GUI
    olc::vf2d screenPos = {engine->ScreenWidth() / 2.0f - 1.5f * renderScale,
                           engine->ScreenHeight() / 2.0f - 0.75f * renderScale};
    unsavedChangesInfoLabel->vPos = screenPos;
    yesButton->vPos = screenPos + olc::vf2d(0.0f, renderScale);
    noButton->vPos = screenPos + olc::vf2d(renderScale, renderScale);
    cancelButton->vPos = screenPos + olc::vf2d(2.0f * renderScale, renderScale);
    // Draw the background of the GUI
    engine->FillRectDecal(
        screenPos,
        {3.0f * renderScale, 1.5f * renderScale},
        backgroundColor);
    guiManager.DrawDecal(engine);
}