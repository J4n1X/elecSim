#include "KeyState.h"
#include <iostream>
#include <format>

namespace Engine {

KeyState::KeyState() {
    keys.fill(false);
}

bool& KeyState::operator[](sf::Keyboard::Key key) {
    unsigned int code = static_cast<unsigned int>(key);
    if (code >= sf::Keyboard::KeyCount) {
        std::cerr << std::format(
            "Warning: Invalid key code {} accessed. Returning reference to first element.\n",
            code);
        return keys[0];
    }
    return keys[code];
}

const bool& KeyState::operator[](sf::Keyboard::Key key) const {
    unsigned int code = static_cast<unsigned int>(key);
    if (code >= sf::Keyboard::KeyCount) {
        std::cerr << std::format(
            "Warning: Invalid key code {} accessed. Returning reference to first element.\n",
            code);
        return keys[0];
    }
    return keys[code];
}

void KeyState::reset() {
    keys.fill(false);
}

void KeyState::setPressed(sf::Keyboard::Key key) {
    unsigned int code = static_cast<unsigned int>(key);
    if (code >= sf::Keyboard::KeyCount) {
        std::cerr << std::format(
            "Warning: Invalid key {} pressed, yielding code {}. Skipping.\n",
            static_cast<int>(key), code);
        return;
    }
    keys[code] = true;
}

void KeyState::setReleased(sf::Keyboard::Key key) {
    unsigned int code = static_cast<unsigned int>(key);
    if (code >= sf::Keyboard::KeyCount) {
        std::cerr << std::format(
            "Warning: Invalid key {} released, yielding code {}. Skipping.\n",
            static_cast<int>(key), code);
        return;
    }
    keys[code] = false;
}

bool KeyState::isPressed(sf::Keyboard::Key key) const {
    return (*this)[key];
}

} // namespace Engine
