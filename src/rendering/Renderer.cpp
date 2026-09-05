#include "rendering/Renderer.h"

#include <cstdint>
#include <format>
#include <utility>

#include "core/Log.h"
#include "core/Noise.h"
#include "narrative/Canon.h"
#include "rendering/PixelFont.h"
#include "simulation/Simulation.h"
#include "ui/Ui.h"

namespace neta::render {

namespace {
constexpr SDL_Color kBg = {0, 0, 0, 255};
constexpr SDL_Color kWhite = {255, 255, 255, 255};
constexpr SDL_Color kYellow = {255, 255, 0, 255};
constexpr SDL_Color kDim = {160, 160, 160, 255};
constexpr SDL_Color kHeart = {255, 0, 0, 255};
constexpr SDL_Color kTrace = {255, 255, 0, 255};
constexpr SDL_Color kRust = {130, 110, 40, 255};

constexpr int kHudH = 26;
constexpr int kFooterH = 44;
constexpr int kMargin = 8;
}  // namespace

bool Renderer::init(const char* title, std::uint64_t seed) {
    if (m_renderer != nullptr) {
        return true;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");  // nearest-neighbor: crisp pixels
    m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr) {
        core::Log::error("render", "SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (m_renderer == nullptr) {
        core::Log::warning("render", "accelerated renderer unavailable, trying software");
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (m_renderer == nullptr) {
        core::Log::error("render", "SDL_CreateRenderer failed: {}", SDL_GetError());
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }
    // Virtual low-res framebuffer; SDL upscales with integer-friendly scaling.
    SDL_RenderSetLogicalSize(m_renderer, kVirtualW, kVirtualH);
    core::Log::info("render", "window opened (seed={}) virtual={}x{}", seed, kVirtualW, kVirtualH);
    return true;
}

void Renderer::shutdown() {
    if (m_renderer != nullptr) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

void Renderer::frameBegin() {
    // Blend ON for the whole frame so translucent panels and scanlines
    // actually blend instead of drawing as solid vision-blocking bars.
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, kBg.r, kBg.g, kBg.b, kBg.a);
    SDL_RenderClear(m_renderer);
}

void Renderer::frameEnd() {
    SDL_RenderPresent(m_renderer);
}

void Renderer::drawText(const std::string& text, int x, int y, int scale, SDL_Color color) {
    PixelFont::drawText(m_renderer, text, x, y, scale, color);
}

void Renderer::fillRect(int x, int y, int w, int h, SDL_Color color) {
    SDL_Rect r{x, y, w, h};
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(m_renderer, &r);
}

void Renderer::drawBox(int x, int y, int w, int h) {
    // Undertale box: solid black + thick white border.
    fillRect(x, y, w, h, kBg);
    SDL_SetRenderDrawColor(m_renderer, kWhite.r, kWhite.g, kWhite.b, kWhite.a);
    for (int t = 0; t < 2; ++t) {
        SDL_Rect border{x + t, y + t, w - 2 * t, h - 2 * t};
        SDL_RenderDrawRect(m_renderer, &border);
    }
}

void Renderer::drawHeart(int x, int y, int scale, SDL_Color color) {
    // 7x6 soul-heart pixels.
    static const char* rows[] = {
        ".XX.XX.", "XXXXXXX", "XXXXXXX", ".XXXXX.", "..XXX..", "...X...",
    };
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (rows[r][c] == 'X') {
                SDL_Rect rect{x + c * scale, y + r * scale, scale, scale};
                SDL_RenderFillRect(m_renderer, &rect);
            }
        }
    }
}

void Renderer::drawWorldArea(const sim::Simulation& sim) {
    const int ax = kMargin;
    const int ay = kHudH;
    const int aw = kVirtualW - 2 * kMargin;
    const int ah = kVirtualH - kHudH - kFooterH - kMargin;

    fillRect(ax, ay, aw, ah, kBg);

    // One sector per screen: the ring shows only where the player stands.
    const int n = sim.world().districtCount() > 0 ? sim.world().districtCount() : 1;
    int here = sim.playerSector();
    if (here < 0) {
        here = 0;
    }
    if (here >= n) {
        here = n - 1;
    }
    const auto& d = sim.world().district(here);

    auto toScreen = [&](entities::Vec2 w) {
        const int sx = ax + static_cast<int>(w.x / sim::Simulation::kAreaW * aw);
        const int sy = ay + static_cast<int>(w.y / sim::Simulation::kAreaH * ah);
        return std::pair<int, int>(sx, sy);
    };
    // Stateless decor hash: same seed + sector = same biome layout, no sim state.
    auto decor = [&](int i, int j) -> int {
        std::uint64_t x = sim.seed() ^ (static_cast<std::uint64_t>(here + 1) * 0x9e3779b97f4a7c15ULL) ^
                          (static_cast<std::uint64_t>(i) * 0x100000ULL +
                           static_cast<std::uint64_t>(j));
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return static_cast<int>(x & 0x7fffffff);
    };

    // Biome art per district theme, dim so gameplay marks pop.
    const std::string& note = d.note;
    const bool blink = (SDL_GetTicks() / 500) % 2 == 0;
    // Noise ground first: organic patches/paths per theme (visual only,
    // deterministic per seed + sector, never touches sim state).
    {
        noise::Preset gp{4, 0.5f, 2.0f, 0.6f, 16.0f};
        float thresh = 0.22f;
        SDL_Color patch = kDim;
        const bool streaks = (note == "antenna fields");
        if (streaks) {
            gp = noise::Preset{3, 0.5f, 2.2f, 0.9f, 22.0f};
            thresh = 0.72f;
        } else if (note == "industrial sector") {
            gp = noise::Preset{5, 0.55f, 2.0f, 0.8f, 13.0f};
            thresh = 0.18f;
            patch = kRust;
        } else if (note == "data haven") {
            gp = noise::Preset{3, 0.5f, 2.4f, 0.4f, 9.0f};
            thresh = 0.3f;
        }
        const std::uint64_t gseed = sim.seed() ^ (static_cast<std::uint64_t>(here + 1) * 0x9e3779b97f4a7c15ULL);
        constexpr int kCell = 6;
        for (int gy = ay + 16; gy < ay + ah - 8; gy += kCell) {
            for (int gx = ax + 6; gx < ax + aw - 8; gx += kCell) {
                const float wx =
                    static_cast<float>(gx - ax) / static_cast<float>(aw) * sim::Simulation::kAreaW;
                const float wy =
                    static_cast<float>(gy - ay) / static_cast<float>(ah) * sim::Simulation::kAreaH;
                const float v = streaks ? noise::ridged(gseed, wx, wy, gp)
                                        : noise::warped(gseed, wx, wy, gp);
                if (v > thresh) {
                    fillRect(gx, gy, 2, 2, patch);
                }
            }
        }
    }
    if (note == "market sprawl") {
        for (int i = 0; i < 12; ++i) {
            const int sx = ax + 12 + decor(i, 0) % (aw - 30);
            const int sy = ay + 26 + decor(i, 1) % (ah - 50);
            fillRect(sx, sy, 7, 4, kDim);
            if (decor(i, 2) % 3 == 0) {
                fillRect(sx + 1, sy - 2, 5, 1, kYellow);
            }
        }
    } else if (note == "antenna fields") {
        for (int i = 0; i < 8; ++i) {
            const int sx = ax + 14 + (i * (aw - 28)) / 7 + decor(i, 0) % 7 - 3;
            const int top = ay + 20 + decor(i, 1) % (ah / 3);
            const int base = ay + ah - 24 - decor(i, 2) % 20;
            SDL_SetRenderDrawColor(m_renderer, kDim.r, kDim.g, kDim.b, kDim.a);
            SDL_RenderDrawLine(m_renderer, sx, top, sx, base);
            if (blink) {
                fillRect(sx - 1, top - 2, 3, 3, kYellow);
            }
        }
    } else if (note == "night clinic row") {
        for (int i = 0; i < 6; ++i) {
            const int sx = ax + 12 + (i * (aw - 30)) / 5;
            const int sy = ay + 30 + decor(i, 0) % (ah - 70);
            SDL_SetRenderDrawColor(m_renderer, kDim.r, kDim.g, kDim.b, kDim.a);
            SDL_Rect r{sx, sy, 10, 8};
            SDL_RenderDrawRect(m_renderer, &r);
            fillRect(sx + 2, sy + 2, 6, 4, blink ? kYellow : kDim);
        }
    } else if (note == "port authority") {
        for (int i = 0; i < 6; ++i) {
            const int sx = ax + 12 + decor(i, 0) % (aw - 40);
            const int sy = ay + 30 + decor(i, 1) % (ah - 60);
            SDL_SetRenderDrawColor(m_renderer, kDim.r, kDim.g, kDim.b, kDim.a);
            SDL_Rect r{sx, sy, 16, 9};
            SDL_RenderDrawRect(m_renderer, &r);
        }
        SDL_SetRenderDrawColor(m_renderer, kWhite.r, kWhite.g, kWhite.b, kWhite.a);
        SDL_Rect gate{ax + aw / 2 - 8, ay + ah - 22, 16, 12};
        SDL_RenderDrawRect(m_renderer, &gate);
    } else if (note == "industrial sector") {
        for (int i = 0; i < 6; ++i) {
            const int sx = ax + 14 + (i * (aw - 28)) / 5;
            SDL_SetRenderDrawColor(m_renderer, kDim.r, kDim.g, kDim.b, kDim.a);
            SDL_RenderDrawLine(m_renderer, sx, ay + 24, sx, ay + ah - 20);
        }
        for (int i = 0; i < 3; ++i) {
            const int sx = ax + 20 + decor(i, 0) % (aw - 50);
            fillRect(sx, ay + ah - 40, 8, 20, kDim);
        }
        // Drifting rain (clock-based, visual only).
        for (int i = 0; i < 12; ++i) {
            const int rx = ax + 10 + decor(i, 5) % (aw - 20);
            const int span = ah - 48;
            const int ry =
                ay + 24 + (span > 0 ? (static_cast<int>(SDL_GetTicks() / 60) + i * 53) % span : 0);
            SDL_SetRenderDrawColor(m_renderer, kDim.r, kDim.g, kDim.b, kDim.a);
            SDL_RenderDrawLine(m_renderer, rx, ry, rx - 1, ry + 3);
        }
    } else if (note == "residential stacks") {
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 8; ++c) {
                const int sx = ax + 16 + c * ((aw - 32) / 7) + decor(r * 8 + c, 0) % 4;
                const int sy = ay + 34 + r * 22;
                fillRect(sx, sy, 9, 7, kDim);
            }
        }
    } else {  // data haven and anything future: twinkling server lights
        for (int i = 0; i < 24; ++i) {
            const int sx = ax + 10 + decor(i, 0) % (aw - 20);
            const int sy = ay + 24 + decor(i, 1) % (ah - 44);
            if ((decor(i, 3) + (blink ? 1 : 0)) % 2 != 0) {
                continue;
            }
            fillRect(sx, sy, 2, 2, (decor(i, 2) % 4 == 0) ? kYellow : kDim);
        }
    }

    // Sector title + minimap ring (visited light up, current in brackets).
    std::string turfShort = "?";
    {
        const int turf = sim.turf(here);
        if (turf >= 0 && static_cast<std::size_t>(turf) < sim.factions().size()) {
            const std::string& fname =
                sim.factions()[static_cast<std::size_t>(turf)].name();
            const std::size_t sp = fname.find(' ');
            turfShort = (sp == std::string::npos) ? fname : fname.substr(0, sp);
        }
    }
    drawText(std::format("SECTOR {}: {} [{}]", here + 1, d.name, turfShort), ax + 4, ay + 3, 1,
             kWhite);
    {
        int mx = ax + aw - 4;
        for (int i = n - 1; i >= 0; --i) {
            const std::string num = std::to_string(i + 1);
            const bool cur = (i == here);
            const bool seen = cur || sim.visited(i) != 0;
            const std::string tok = cur ? "[" + num + "]" : " " + num + " ";
            const int w = static_cast<int>(tok.size()) * 4;
            mx -= w;
            drawText(tok, mx, ay + 3, 1, seen ? kWhite : kDim);
            mx -= 2;
        }
    }

    // Traces: yellow squares, blink when not found (this sector only).
    for (const auto& t : sim.traces()) {
        if (t.found || t.district != here) {
            continue;
        }
        if (!blink) {
            continue;
        }
        const auto [sx, sy] = toScreen(t.pos);
        fillRect(sx - 2, sy - 2, 5, 5, kTrace);
    }

    // Erase terminal: white door (this sector only).
    if (sim.eraseTerminal().district == here) {
        const auto [sx, sy] = toScreen(sim.eraseTerminal().pos);
        fillRect(sx - 3, sy - 4, 7, 9, kWhite);
        fillRect(sx - 2, sy - 3, 5, 7, kBg);
        if (blink) {
            drawText("ERASE", sx - 12, sy + 7, 1, kWhite);
        }
    }

    // Places: dim dots + names (this sector only, room for all now).
    for (const auto& loc : sim.world().locations()) {
        if (loc.district != here) {
            continue;
        }
        entities::Vec2 w{loc.x, loc.y};
        const auto [sx, sy] = toScreen(w);
        fillRect(sx - 1, sy - 1, 2, 2, kDim);
        drawText(loc.name, sx + 3, sy - 3, 1, kDim);
    }

    // NPCs: small white squares, this sector only. Messenger yellow.
    for (std::size_t i = 0; i < sim.npcs().size(); ++i) {
        const auto wp = sim.npcLocalPos(i);
        if (wp.x < -50.0f) {
            continue;  // hidden messenger or other sector
        }
        const auto [sx, sy] = toScreen(wp);
        SDL_Color c = kWhite;
        if (i < 5 && sim.messengerSpawned() && i == 4) {
            c = kYellow;
        }
        fillRect(sx - 1, sy - 1, 3, 3, c);
    }

    // Player: red soul-heart.
    {
        const auto [sx, sy] = toScreen(sim.player().position());
        drawHeart(sx - 3, sy - 3, 1, kHeart);
    }

    // Sector character line (bottom-left of world area).
    {
        const char* rich = d.wealth > 66 ? "RICH" : (d.wealth < 33 ? "POOR" : "MIXED");
        const char* busy = d.bustle > 66 ? "BUSY" : (d.bustle < 33 ? "QUIET" : "STEADY");
        drawText(std::format("* SECTOR {}: {} [{}] - {} ({}/{})", here + 1, d.name, turfShort,
                             d.note, rich, busy),
                 ax + 4, ay + ah - 10, 1, kWhite);
    }

    // Outer frame: border only (no fill -- fill would cover the districts).
    SDL_SetRenderDrawColor(m_renderer, kWhite.r, kWhite.g, kWhite.b, kWhite.a);
    for (int t = 0; t < 2; ++t) {
        SDL_Rect frame{ax + t, ay + t, aw - 2 * t, ah - 2 * t};
        SDL_RenderDrawRect(m_renderer, &frame);
    }

    // Footer: objective + stage, Undertale colors.
    const std::string obj =
        std::format("* {} [{}]", canon::kObjective,
                    narrative::NarrativeState::stageName(sim.narrative().stage()));
    drawText(obj, kMargin, kVirtualH - kFooterH + 5, 1, kYellow);
    drawText("* WASD MOVE / E TALK / EDGE = TRAVEL / F1 LOG / F5 SAVE / ESC QUIT", kMargin,
             kVirtualH - kFooterH + 20, 1, kDim);
}

void Renderer::drawScanlines() {
    // Subtle CRT hint: faint lines every 4th row, not vision-blocking bars.
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 28);
    for (int y = 0; y < kVirtualH; y += 4) {
        SDL_RenderDrawLine(m_renderer, 0, y, kVirtualW, y);
    }
}

void Renderer::render(const sim::Simulation& sim, const UiOptions& ui, double fps) {
    if (!ready()) {
        return;
    }
    m_scanlines = ui.scanlines;
    frameBegin();
    drawWorldArea(sim);
    Ui::draw(*this, sim, ui, fps);
    if (m_scanlines) {
        drawScanlines();
    }
    frameEnd();
}

}  // namespace neta::render
