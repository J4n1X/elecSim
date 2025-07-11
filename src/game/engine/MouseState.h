#pragma once

#include <array>
#include "SFML/Window/Mouse.hpp"

namespace Engine {

/**
 * @class MouseState
 * @brief Manages the state of mouse buttons.
 *
 * This class provides an interface to track and manipulate the pressed/released state
 * of all mouse buttons supported by SFML. It allows querying, setting, and resetting
 * the state of each button, as well as convenient indexing using SFML's mouse button enum.
 *
 * @note Internally, the state is stored as an array of booleans, one for each button.
 */
class MouseState {
public:
    MouseState();
    
    // Allow indexing with SFML mouse buttons directly
    bool& operator[](sf::Mouse::Button button);
    const bool& operator[](sf::Mouse::Button button) const;
    
    // Reset all buttons to released state
    void Reset();
    
    void SetPressed(sf::Mouse::Button button);
    void SetReleased(sf::Mouse::Button button);
    
    // Check if button is pressed
    bool IsPressed(sf::Mouse::Button button) const;

private:
    std::array<bool, sf::Mouse::ButtonCount> buttons;
};

} // namespace Engine
