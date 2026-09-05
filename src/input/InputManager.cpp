#include "input/InputManager.h"

namespace neta::input {

void InputManager::pump() {
    m_pressed.clear();
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            m_quit = true;
        } else if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
            m_pressed.insert(static_cast<int>(e.key.keysym.scancode));
            if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                m_quit = true;
            }
        }
    }
}

bool InputManager::isHeld(SDL_Scancode code) const {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    return keys != nullptr && keys[code] != 0;
}

bool InputManager::wasPressed(SDL_Scancode code) const {
    return m_pressed.count(static_cast<int>(code)) > 0;
}

float InputManager::axisX() const {
    float x = 0.0f;
    if (isHeld(SDL_SCANCODE_A) || isHeld(SDL_SCANCODE_LEFT)) {
        x -= 1.0f;
    }
    if (isHeld(SDL_SCANCODE_D) || isHeld(SDL_SCANCODE_RIGHT)) {
        x += 1.0f;
    }
    return x;
}

float InputManager::axisY() const {
    float y = 0.0f;
    if (isHeld(SDL_SCANCODE_W) || isHeld(SDL_SCANCODE_UP)) {
        y -= 1.0f;
    }
    if (isHeld(SDL_SCANCODE_S) || isHeld(SDL_SCANCODE_DOWN)) {
        y += 1.0f;
    }
    return y;
}

}  // namespace neta::input
