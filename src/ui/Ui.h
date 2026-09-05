#pragma once

#include "rendering/Renderer.h"

namespace neta::sim {
class Simulation;
}

namespace neta {

// Immediate-mode debug/HUD overlay. Reads Simulation + primitives from
// Renderer; owns no state. Future menus/dialogue/terminal UI grow from here.
class Ui {
public:
    static void draw(render::Renderer& renderer, const sim::Simulation& sim, const UiOptions& ui,
                     double fps);
};

}  // namespace neta
