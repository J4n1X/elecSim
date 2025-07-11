#include "KeyState.h"
#include <iostream>
#include <format>
#include <stdexcept>

namespace Engine {

KeyState::KeyState() {
    keys.fill(false);
}

bool& KeyState::operator[](sf::Keyboard::Key key) {
    unsigned int code = static_cast<unsigned int>(key);
    if (code >= sf::Keyboard::KeyCount) {
        throw std::out_of_range(std::format(
            "Invalid key code {} accessed. Valid range is 0-{}.",
            code, sf::Keyboard::KeyCount - 1));
    }
    return keys[code];
}

const bool& KeyState::operator[](sf::Keyboard::Key key) const {
    unsigned int code = static_cast<unsigned int>(key);
    if (code >= sf::Keyboard::KeyCount) {
        throw std::out_of_range(std::format(
            "Invalid key code {} accessed. Valid range is 0-{}.",
            code, sf::Keyboard::KeyCount - 1));
    }
    return keys[code];
}

void KeyState::Reset() {
    keys.fill(false);
}

void KeyState::SetPressed(sf::Keyboard::Key key) {
    unsigned int code = static_cast<unsigned int>(key);
    if (code >= sf::Keyboard::KeyCount) {
        std::cerr << std::format(
            "Warning: Invalid key {} pressed, yielding code {}. Skipping.\n",
            static_cast<int>(key), code);
        return;
    }
    keys[code] = true;
}

void KeyState::SetReleased(sf::Keyboard::Key key) {
    unsigned int code = static_cast<unsigned int>(key);
    if (code >= sf::Keyboard::KeyCount) {
        std::cerr << std::format(
            "Warning: Invalid key {} released, yielding code {}. Skipping.\n",
            static_cast<int>(key), code);
        return;
    }
    keys[code] = false;
}

bool KeyState::IsPressed(sf::Keyboard::Key key) const {
    return (*this)[key];
}

} // namespace Engine
