#include "ChoiceMessageBox.h"
#include <imgui.h>
#include <imgui-SFML.h>

namespace Engine {

ChoiceMessageBox::ChoiceMessageBox() 
    : visible(false), result(ChoiceMessageBoxResult::None) {
}

void ChoiceMessageBox::Show(const std::string& message) {
    visible = true;
    dialogMessage = message;
    result = ChoiceMessageBoxResult::None;
}

void ChoiceMessageBox::Hide() {
    visible = false;
    result = ChoiceMessageBoxResult::None;
}

bool ChoiceMessageBox::IsVisible() const {
    return visible;
}

ChoiceMessageBoxResult ChoiceMessageBox::Render() {
    if (!visible) {
        return ChoiceMessageBoxResult::None;
    }

    ImGui::OpenPopup("Unsaved Changes");
    
    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", dialogMessage.c_str());
        ImGui::Separator();
        
        if (ImGui::Button("Yes", ImVec2(80, 0))) {
            result = ChoiceMessageBoxResult::Yes;
            if (onSaveCallback) {
                onSaveCallback();
            }
            Hide();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(80, 0))) {
            result = ChoiceMessageBoxResult::No;
            if (onProceedCallback) {
                onProceedCallback();
            }
            Hide();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            result = ChoiceMessageBoxResult::Cancel;
            if (onCancelCallback) {
                onCancelCallback();
            }
            Hide();
        }
        
        ImGui::EndPopup();
    }
    
    return result;
}

void ChoiceMessageBox::SetOnSaveCallback(std::function<void()> callback) {
    onSaveCallback = callback;
}

void ChoiceMessageBox::SetOnProceedCallback(std::function<void()> callback) {
    onProceedCallback = callback;
}

void ChoiceMessageBox::SetOnCancelCallback(std::function<void()> callback) {
    onCancelCallback = callback;
}

}  // namespace Engine