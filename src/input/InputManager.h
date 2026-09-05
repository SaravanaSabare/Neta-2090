#pragma once

#include <unordered_set>

#include <SDL.h>

namespace neta::input {

// Keyboard + window-event pump. One pump() per frame: drains SDL events,
// records edge-triggered presses, and exposes held-key state for movement.
// Game acts on this; the class never touches Simulation directly.
class InputManager {
public:
    void pump();

    bool quitRequested() const { return m_quit; }
    bool isHeld(SDL_Scancode code) const;
    bool wasPressed(SDL_Scancode code) const;

    // -1..1 movement axes from WASD + arrow keys.
    float axisX() const;
    float axisY() const;

private:
    bool m_quit = false;
    std::unordered_set<int> m_pressed;  // edge-triggered, cleared each pump
};

}  // namespace neta::input
