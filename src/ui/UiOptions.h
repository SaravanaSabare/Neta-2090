#pragma once

#include <cstdint>
#include <string>

#include <SDL.h>

namespace neta::render {

class Renderer;  // defined in rendering/Renderer.h

}  // namespace neta::render

// UI overlay options. Defined here (not in Renderer.h) so ui/ and rendering/
// depend in one direction only: Renderer -> Ui options, Ui -> Renderer.
struct UiOptions {
    bool showDebug = true;
    bool scanlines = false;
    bool paused = false;
    // Undertale-style flow owned by Game, drawn by Ui.
    bool showTitle = true;
    bool showWin = false;
    bool dlgOpen = false;
    std::string dlgSpeaker;
    std::string dlgText;
    std::size_t dlgShown = 0;
    int tracesFound = 0;
    int tracesTotal = 3;
};
