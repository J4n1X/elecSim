#pragma once

#include <array>
#include "SFML/Window/Keyboard.hpp"

namespace Engine {

class KeyState {
public:
    KeyState();
    
    // Allow indexing with SFML keys directly
    bool& operator[](sf::Keyboard::Key key);
    const bool& operator[](sf::Keyboard::Key key) const;
    
    // Reset all keys to released state
    void reset();
    
    // Set key state
    void setPressed(sf::Keyboard::Key key);
    void setReleased(sf::Keyboard::Key key);
    
    // Check if key is pressed
    bool isPressed(sf::Keyboard::Key key) const;

private:
    std::array<bool, sf::Keyboard::KeyCount> keys;
};

} // namespace Engine
