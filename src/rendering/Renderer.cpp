#include "rendering/Renderer.h"

#include <format>
#include <utility>

#include "core/Log.h"
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

    // Districts as plain black columns with thin white borders.
    const int n = sim.world().districtCount() > 0 ? sim.world().districtCount() : 1;
    const int colW = aw / n;
    for (int i = 0; i < sim.world().districtCount(); ++i) {
        const int cx = ax + i * colW;
        const int cw = (i == n - 1) ? (ax + aw - cx) : colW;
        SDL_SetRenderDrawColor(m_renderer, kWhite.r, kWhite.g, kWhite.b, kWhite.a);
        SDL_Rect border{cx, ay, cw, ah};
        SDL_RenderDrawRect(m_renderer, &border);
        drawText(sim.world().district(i).name, cx + 4, ay + 3, 1, kDim);
    }
    auto toScreen = [&](entities::Vec2 w) {
        const int sx = ax + static_cast<int>(w.x / sim::Simulation::kAreaW * aw);
        const int sy = ay + static_cast<int>(w.y / sim::Simulation::kAreaH * ah);
        return std::pair<int, int>(sx, sy);
    };

    const bool blink = (SDL_GetTicks() / 500) % 2 == 0;

    // Traces: yellow squares, blink when not found.
    for (const auto& t : sim.traces()) {
        if (t.found) {
            continue;
        }
        if (!blink) {
            continue;
        }
        const auto [sx, sy] = toScreen(t.pos);
        fillRect(sx - 2, sy - 2, 5, 5, kTrace);
    }

    // Erase terminal: white door.
    {
        const auto [sx, sy] = toScreen(sim.eraseTerminal().pos);
        fillRect(sx - 3, sy - 4, 7, 9, kWhite);
        fillRect(sx - 2, sy - 3, 5, 7, kBg);
        if (blink) {
            drawText("ERASE", sx - 12, sy + 7, 1, kWhite);
        }
    }

    // Places: dim dots everywhere, names only in the player's district.
    {
        const int nLoc = sim.world().districtCount() > 0 ? sim.world().districtCount() : 1;
        int here = static_cast<int>(sim.player().position().x / sim::Simulation::kAreaW * nLoc);
        if (here < 0) {
            here = 0;
        }
        if (here >= nLoc) {
            here = nLoc - 1;
        }
        for (const auto& loc : sim.world().locations()) {
            entities::Vec2 w{loc.x, loc.y};
            const auto [sx, sy] = toScreen(w);
            fillRect(sx - 1, sy - 1, 2, 2, kDim);
            if (loc.district == here) {
                drawText(loc.name, sx + 3, sy - 3, 1, kDim);
            }
        }
        // Current district label with character (bottom-left of world area).
        const auto& d = sim.world().district(here);
        const char* rich = d.wealth > 66 ? "RICH" : (d.wealth < 33 ? "POOR" : "MIXED");
        const char* busy = d.bustle > 66 ? "BUSY" : (d.bustle < 33 ? "QUIET" : "STEADY");
        drawText(std::format("* {} - {} ({}/{})", d.name, d.note, rich, busy), ax + 4,
                 ay + ah - 10, 1, kWhite);
    }

    // NPCs: small white squares. Messenger (index 4) yellow when spawned.
    for (std::size_t i = 0; i < sim.npcs().size(); ++i) {
        const auto wp = sim.npcWorldPos(i);
        if (wp.x < -50.0f) {
            continue;  // hidden messenger
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
    drawText("* WASD MOVE / E TALK / F1 LOG / F5 SAVE / F9 LOAD / ESC QUIT", kMargin,
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
