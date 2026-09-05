#pragma once

#include <string>

#include <SDL.h>

// Tiny built-in 3x5 pixel font so HUD/debug text needs no font library
// (SDL_ttf would add a dependency for zero gameplay value right now).
// Renders capitals, digits, and common punctuation; lowercase is mapped up.
class PixelFont {
public:
    static void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale,
                         SDL_Color color);
    static int textWidth(const std::string& text, int scale);
    static constexpr int kGlyphW = 3;
    static constexpr int kGlyphH = 5;
};
