#pragma once

#include <array>
#include "SFML/Window/Mouse.hpp"

namespace Engine {

class MouseState {
public:
    MouseState();
    
    // Allow indexing with SFML mouse buttons directly
    bool& operator[](sf::Mouse::Button button);
    const bool& operator[](sf::Mouse::Button button) const;
    
    // Reset all buttons to released state
    void reset();
    
    // Set button state
    void setPressed(sf::Mouse::Button button);
    void setReleased(sf::Mouse::Button button);
    
    // Check if button is pressed
    bool isPressed(sf::Mouse::Button button) const;

private:
    std::array<bool, sf::Mouse::ButtonCount> buttons;
};

} // namespace Engine
