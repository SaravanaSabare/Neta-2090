#pragma once

#include <memory>

#include "core/Config.h"
#include "platform/Steam.h"

namespace neta::core {

// Application lifecycle: owns the sim/render split. Headless mode ticks the
// Simulation with no SDL objects at all; windowed mode adds Renderer, input,
// and audio around the same Simulation. All Steam access goes through the
// provider so the game always runs offline in development.
class Game {
public:
    explicit Game(Config config)
        : m_config(config) {}

    int run();

private:
    int runHeadless();
    int runWindowed();

    Config m_config;
    std::unique_ptr<platform::ISteamProvider> m_steam;
};

}  // namespace neta::core
