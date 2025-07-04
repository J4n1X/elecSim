#pragma once

#include <array>
#include "SFML/Window/Keyboard.hpp"

namespace Engine {

/**
 * @class KeyState
 * @brief Manages keyboard input state for the game engine.
 */
class KeyState {
public:
    KeyState();
    
    bool& operator[](sf::Keyboard::Key key);
    const bool& operator[](sf::Keyboard::Key key) const;
    
    void Reset();
    void SetPressed(sf::Keyboard::Key key);
    void SetReleased(sf::Keyboard::Key key);
    bool IsPressed(sf::Keyboard::Key key) const;

private:
    std::array<bool, sf::Keyboard::KeyCount> keys;
};

} // namespace Engine
