#pragma once

#include <functional>
#include <string>

namespace Engine {

enum class ChoiceMessageBoxResult {
    None,
    Yes,
    No,
    Cancel
};

class ChoiceMessageBox {
public:
    ChoiceMessageBox();
    ~ChoiceMessageBox() = default;

    void Show(const std::string& message = "You have unsaved changes. Do you want to save before continuing?");
    void Hide();
    bool IsVisible() const;
    
    ChoiceMessageBoxResult Render();

    void SetOnSaveCallback(std::function<void()> callback);
    void SetOnProceedCallback(std::function<void()> callback);
    void SetOnCancelCallback(std::function<void()> callback);

private:
    bool visible;
    std::string dialogMessage;
    ChoiceMessageBoxResult result;
    
    std::function<void()> onSaveCallback;
    std::function<void()> onProceedCallback;
    std::function<void()> onCancelCallback;
};

}  // namespace Engine