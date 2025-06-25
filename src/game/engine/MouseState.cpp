#include "MouseState.h"
#include <iostream>
#include <format>
#include <stdexcept>

namespace Engine {

MouseState::MouseState() {
    buttons.fill(false);
}

bool& MouseState::operator[](sf::Mouse::Button button) {
    unsigned int code = static_cast<unsigned int>(button);
    if (code >= sf::Mouse::ButtonCount) {
        throw std::out_of_range(std::format(
            "Invalid mouse button code {} accessed. Valid range is 0-{}.",
            code, sf::Mouse::ButtonCount - 1));
    }
    return buttons[code];
}

const bool& MouseState::operator[](sf::Mouse::Button button) const {
    unsigned int code = static_cast<unsigned int>(button);
    if (code >= sf::Mouse::ButtonCount) {
        throw std::out_of_range(std::format(
            "Invalid mouse button code {} accessed. Valid range is 0-{}.",
            code, sf::Mouse::ButtonCount - 1));
    }
    return buttons[code];
}

void MouseState::reset() {
    buttons.fill(false);
}

void MouseState::setPressed(sf::Mouse::Button button) {
    unsigned int code = static_cast<unsigned int>(button);
    if (code >= sf::Mouse::ButtonCount) {
        std::cerr << std::format(
            "Warning: Invalid mouse button {} pressed, yielding code {}. Skipping.\n",
            static_cast<int>(button), code);
        return;
    }
    buttons[code] = true;
}

void MouseState::setReleased(sf::Mouse::Button button) {
    unsigned int code = static_cast<unsigned int>(button);
    if (code >= sf::Mouse::ButtonCount) {
        std::cerr << std::format(
            "Warning: Invalid mouse button {} released, yielding code {}. Skipping.\n",
            static_cast<int>(button), code);
        return;
    }
    buttons[code] = false;
}

bool MouseState::isPressed(sf::Mouse::Button button) const {
    return (*this)[button];
}

} // namespace Engine
