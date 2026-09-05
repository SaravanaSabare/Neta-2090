#include "core/Game.h"

#include <format>

#include <SDL.h>

#include "audio/AudioSystem.h"
#include "core/Clock.h"
#include "core/Log.h"
#include "core/Random.h"
#include "input/InputManager.h"
#include "narrative/OneShot.h"
#include "rendering/Renderer.h"
#include "save/SaveSystem.h"
#include "simulation/Simulation.h"
#include "ui/Ui.h"

#ifndef NETA_VERSION_STRING
#define NETA_VERSION_STRING "0.0.0-dev"
#endif

namespace neta::core {

int Game::run() {
    Log::init(m_config.verbose);
    Log::info("game", "NETA foundation v{} starting (seed={})", NETA_VERSION_STRING, m_config.seed);

    // Determinism receipt: anyone can re-derive these numbers by hand from
    // the seed, proving which RNG produced this run's world.
    Random rng(m_config.seed);
    RngStream boot = rng.streamFor("boot");
    Log::info("seed", "master={} boot-stream={} {} {}", m_config.seed, boot.nextU64(),
              boot.nextU64(), boot.nextU64());

    m_steam = std::make_unique<platform::NullSteamProvider>();
    m_steam->init();

    if (m_config.headless) {
        return runHeadless();
    }
    return runWindowed();
}

int Game::runHeadless() {
    sim::Simulation sim;
    sim.generate(m_config.seed);
    if (!m_config.loadPath.empty()) {
        save::SaveData data;
        std::string error;
        if (!save::SaveSystem::loadFromFile(m_config.loadPath, data, error)) {
            Log::error("game", "load failed: {}", error);
            return 1;
        }
        if (!sim.restore(data, error)) {
            Log::error("game", "restore failed: {}", error);
            return 1;
        }
    }

    std::printf("WORLD GENERATED seed=%llu districts=%d places=%zu factions=%zu npcs=%zu traces=%d\n",
                static_cast<unsigned long long>(sim.seed()), sim.world().districtCount(),
                sim.world().locations().size(), sim.factions().size(), sim.npcs().size(),
                sim.tracesFound());
    for (int i = 0; i < m_config.headlessTicks; ++i) {
        sim.tick(sim::Simulation::kTickDt);
        // Spec-style debug feed: one line per tick a future debugger can diff.
        std::printf("TICK %llu t=%.1f stage=%s events=%zu player=(%.1f,%.1f) traces=%d won=%d\n",
                    static_cast<unsigned long long>(sim.tickCount()), sim.timeSeconds(),
                    narrative::NarrativeState::stageName(sim.narrative().stage()),
                    sim.eventLog().size(), sim.player().position().x, sim.player().position().y,
                    sim.tracesFound(), sim.won() ? 1 : 0);
        if (!sim.eventLog().empty()) {
            std::printf("  last: %s\n", sim.eventLog().back().c_str());
        }
    }
    std::printf("DONE ticks=%llu\n", static_cast<unsigned long long>(sim.tickCount()));
    return 0;
}

int Game::runWindowed() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        Log::error("game", "SDL_Init failed: {}", SDL_GetError());
        return 1;
    }

    int exitCode = 0;
    {
        sim::Simulation sim;
        sim.generate(m_config.seed);
        if (!m_config.loadPath.empty()) {
            save::SaveData data;
            std::string error;
            if (save::SaveSystem::loadFromFile(m_config.loadPath, data, error) &&
                sim.restore(data, error)) {
                Log::info("game", "loaded save '{}'", m_config.loadPath);
            } else {
                Log::error("game", "load failed: {}", error);
            }
        }

        render::Renderer renderer;
        const std::string title =
            std::format("NETA-2090 [seed {}] - one-shot", m_config.seed);
        if (!renderer.init(title.c_str(), m_config.seed)) {
            exitCode = 1;
        } else {
            input::InputManager input;
            audio::AudioSystem audio;
            audio.init();
            UiOptions ui;
            ui.showTitle = true;
            ui.showDebug = false;
            ui.scanlines = false;
            bool started = false;
            bool pendingWin = false;
            auto openDlg = [&](const std::string& speaker, const std::string& text) {
                ui.dlgOpen = true;
                ui.dlgSpeaker = speaker;
                ui.dlgText = text;
                ui.dlgShown = 0;
            };
            auto syncProgress = [&]() {
                ui.tracesFound = sim.tracesFound();
                ui.tracesTotal = oneshot::kTraceCount;
            };
            // If we loaded a save with progress, skip the title.
            if (!m_config.loadPath.empty() && (sim.tracesFound() > 0 || sim.won())) {
                started = true;
                ui.showTitle = false;
                if (sim.won()) {
                    ui.showWin = true;
                }
            }
            syncProgress();
            Clock clock;
            double accumulator = 0.0;
            double fps = 60.0;
            const std::string quickPath = save::SaveSystem::defaultSavePath("quicksave.nsave");

            while (!input.quitRequested()) {
                clock.tick();
                const double dt = clock.deltaSeconds();
                if (dt > 0.0) {
                    fps += (1.0 / dt - fps) * 0.05;
                }

                input.pump();
                const bool advPressed = input.wasPressed(SDL_SCANCODE_E) ||
                                        input.wasPressed(SDL_SCANCODE_Z) ||
                                        input.wasPressed(SDL_SCANCODE_RETURN) ||
                                        input.wasPressed(SDL_SCANCODE_KP_ENTER);
                const bool startPressed = advPressed || input.wasPressed(SDL_SCANCODE_SPACE);
                if (input.wasPressed(SDL_SCANCODE_F1)) {
                    ui.showDebug = !ui.showDebug;
                }
                if (input.wasPressed(SDL_SCANCODE_F2)) {
                    ui.scanlines = !ui.scanlines;
                }
                if (input.wasPressed(SDL_SCANCODE_F5)) {
                    save::SaveSystem::saveToFile(quickPath, sim.snapshot());
                }
                if (input.wasPressed(SDL_SCANCODE_F9)) {
                    save::SaveData data;
                    std::string error;
                    if (save::SaveSystem::loadFromFile(quickPath, data, error) &&
                        sim.restore(data, error)) {
                        Log::info("game", "quicksave loaded");
                        started = true;
                        ui.showTitle = false;
                        ui.dlgOpen = false;
                        pendingWin = false;
                        ui.showWin = sim.won();
                        syncProgress();
                    } else {
                        Log::warning("game", "quickload failed: {}", error);
                    }
                }
                if (input.quitRequested()) {
                    break;
                }

                // Title -> dialogue -> world flow (Undertale-like: text pauses sim).
                if (ui.showTitle) {
                    if (startPressed) {
                        started = true;
                        ui.showTitle = false;
                        openDlg("", oneshot::introText());
                        syncProgress();
                    }
                } else if (ui.showWin) {
                    // Won: world stays still, ESC quits.
                } else if (ui.dlgOpen) {
                    // Typewriter advance.
                    if (ui.dlgShown < ui.dlgText.size()) {
                        ui.dlgShown += 2;
                        if (ui.dlgShown > ui.dlgText.size()) {
                            ui.dlgShown = ui.dlgText.size();
                        }
                    }
                    if (advPressed || input.wasPressed(SDL_SCANCODE_SPACE)) {
                        if (ui.dlgShown < ui.dlgText.size()) {
                            ui.dlgShown = ui.dlgText.size();
                        } else {
                            ui.dlgOpen = false;
                            if (pendingWin) {
                                pendingWin = false;
                                ui.showWin = true;
                            }
                        }
                    }
                } else {
                    if (input.wasPressed(SDL_SCANCODE_SPACE)) {
                        ui.paused = !ui.paused;
                        Log::infoText("game", ui.paused ? "simulation paused" : "simulation resumed");
                    }
                    if (advPressed) {
                        sim::Simulation::InteractResult r = sim.interact();
                        if (r.kind != sim::Simulation::InteractKind::None) {
                            openDlg(r.speaker, r.text);
                            if (r.kind == sim::Simulation::InteractKind::EraseWin) {
                                pendingWin = true;
                            }
                            syncProgress();
                        }
                    }
                }

                const bool simHalted = !started || ui.dlgOpen || ui.showWin || ui.showTitle;
                if (!ui.paused && !simHalted) {
                    // Queue render-rate intent; fixed-step ticks consume it, so
                    // sim results depend on tick count, not framerate.
                    sim.setPlayerIntent({input.axisX(), input.axisY()});
                    accumulator += dt;
                    int steps = 0;
                    while (accumulator >= sim::Simulation::kTickDt && steps < 5) {
                        sim.tick(sim::Simulation::kTickDt);
                        accumulator -= sim::Simulation::kTickDt;
                        ++steps;
                    }
                    if (steps == 5) {
                        accumulator = 0.0;  // drop backlog after hitches
                    }
                } else {
                    accumulator = 0.0;
                }
                syncProgress();

                audio.update();
                renderer.render(sim, ui, fps);
            }
            audio.shutdown();
            renderer.shutdown();
        }
    }

    SDL_Quit();
    Log::info("game", "shutdown clean");
    return exitCode;
}

}  // namespace neta::core
