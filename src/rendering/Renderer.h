#pragma once

#include <cstdint>
#include <string>

#include <SDL.h>

#include "ui/UiOptions.h"

namespace neta::sim {
class Simulation;
}

namespace neta::render {

// SDL2 presentation layer. Owns the window + renderer and draws a low-res
// virtual framebuffer (480x270, nearest-neighbor upscale) to establish the
// retro pipeline: future pixel art, palettes, and CRT effects slot in here.
// Reads Simulation state every frame; never writes it.
class Renderer {
public:
    static constexpr int kVirtualW = 480;
    static constexpr int kVirtualH = 270;

    Renderer() = default;
    ~Renderer() { shutdown(); }
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(const char* title, std::uint64_t seed);
    void shutdown();
    bool ready() const { return m_renderer != nullptr; }

    void render(const sim::Simulation& sim, const UiOptions& ui, double fps);

    // Primitives used by ui/Ui.
    void drawText(const std::string& text, int x, int y, int scale, SDL_Color color);
    void fillRect(int x, int y, int w, int h, SDL_Color color);
    // Undertale-style box: black fill + thick white border.
    void drawBox(int x, int y, int w, int h);
    // Small soul-heart (player avatar). Pixel shape, no assets.
    void drawHeart(int x, int y, int scale, SDL_Color color);

private:
    void frameBegin();
    void frameEnd();
    void drawWorldArea(const sim::Simulation& sim);
    void drawScanlines();

    bool m_scanlines = true;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
};

constexpr SDL_Color makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                              std::uint8_t a = 255) {
    return SDL_Color{r, g, b, a};
}

}  // namespace neta::render
