#include "ui/Ui.h"

#include <format>
#include <string>
#include <vector>

#include "rendering/Renderer.h"
#include "simulation/Simulation.h"

namespace neta {

namespace {

// Undertale palette: white text, yellow names, dim hints.
constexpr SDL_Color kText = {255, 255, 255, 255};
constexpr SDL_Color kYellow = {255, 255, 0, 255};
constexpr SDL_Color kDim = {160, 160, 160, 255};

// Wrap visible text to maxChars per line, keeping words whole when easy.
std::vector<std::string> wrapText(const std::string& text, std::size_t maxChars) {
    std::vector<std::string> lines;
    std::string cur;
    std::string word;
    auto flushWord = [&]() {
        if (word.empty()) {
            return;
        }
        if (cur.empty()) {
            cur = word;
        } else if (cur.size() + 1 + word.size() <= maxChars) {
            cur += ' ';
            cur += word;
        } else {
            lines.push_back(cur);
            cur = word;
        }
        word.clear();
    };
    for (char c : text) {
        if (c == ' ') {
            flushWord();
        } else {
            word.push_back(c);
            if (word.size() >= maxChars) {
                flushWord();
            }
        }
    }
    flushWord();
    if (!cur.empty()) {
        lines.push_back(cur);
    }
    if (lines.empty()) {
        lines.emplace_back("");
    }
    return lines;
}

}  // namespace

void Ui::draw(render::Renderer& renderer, const sim::Simulation& sim, const UiOptions& ui,
              double fps) {
    // Top HUD box: always visible (even on title, behind it).
    renderer.drawBox(8, 2, 464, 22);
    const std::string hud =
        std::format("NETA-2090  TRACES {}/{}  {}  {:.0f}FPS{}", ui.tracesFound, ui.tracesTotal,
                    narrative::NarrativeState::stageName(sim.narrative().stage()), fps,
                    ui.paused ? "  PAUSED" : "");
    renderer.drawText(hud, 14, 8, 1, kText);

    if (ui.showTitle) {
        renderer.drawBox(40, 60, 400, 140);
        renderer.drawText("NETA-2090", 60, 75, 3, kText);
        renderer.drawText("* YEAR 2090. AN APP THAT SHOULD NOT EXIST.", 60, 115, 1, kDim);
        renderer.drawText("* FIND 3 TRACES. LEARN WHO LIES.", 60, 128, 1, kDim);
        renderer.drawText("* THEN ERASE IT.", 60, 141, 1, kYellow);
        renderer.drawText("* PRESS E TO START", 60, 165, 1, kText);
        renderer.drawText("* WASD MOVE / E TALK", 60, 180, 1, kDim);
        return;
    }

    if (ui.showWin) {
        renderer.drawBox(60, 70, 360, 120);
        renderer.drawText("* APPLICATION ERASED?", 80, 85, 1, kYellow);
        renderer.drawText("* HISTORY FEELS QUIET...", 80, 100, 1, kText);
        renderer.drawText("* BUT IS IT?", 80, 113, 1, kText);
        renderer.drawText(std::format("* TIME {:.1f}S  TICK {}", sim.timeSeconds(), sim.tickCount()),
                          80, 130, 1, kDim);
        renderer.drawText("* PRESS ESC TO QUIT", 80, 150, 1, kDim);
        renderer.drawText("* PRESS F5 TO SAVE THIS ENDING", 80, 163, 1, kDim);
        return;
    }

    if (ui.dlgOpen) {
        constexpr int kBoxX = 8;
        constexpr int kBoxW = 464;
        constexpr int kBoxH = 80;
        constexpr int kBoxY = 270 - 44 - 8 - 80;
        renderer.drawBox(kBoxX, kBoxY, kBoxW, kBoxH);
        if (!ui.dlgSpeaker.empty()) {
            renderer.drawText("* " + ui.dlgSpeaker, kBoxX + 8, kBoxY + 6, 1, kYellow);
        }
        std::string visible = ui.dlgText.substr(0, ui.dlgShown);
        const auto lines = wrapText(visible, 52);
        int y = kBoxY + (ui.dlgSpeaker.empty() ? 8 : 20);
        for (std::size_t i = 0; i < lines.size() && i < 4; ++i) {
            renderer.drawText(lines[i], kBoxX + 8, y, 1, kText);
            y += 12;
        }
        if (ui.dlgShown >= ui.dlgText.size()) {
            const bool blink = (SDL_GetTicks() / 500) % 2 == 0;
            if (blink) {
                renderer.drawText("E >", kBoxX + kBoxW - 32, kBoxY + kBoxH - 14, 1, kYellow);
            }
        }
    }

    if (!ui.showDebug) {
        return;
    }
    // Debug log as a small black box. Hidden while dialogue is open so the
    // talk box is always readable.
    if (ui.dlgOpen) {
        return;
    }
    const auto& events = sim.eventLog();
    const int lines = static_cast<int>(events.size() > 4 ? 4 : events.size());
    if (lines == 0) {
        return;
    }
    const int panelH = 12 + lines * 8;
    constexpr int kFooterH = 44;
    const int panelY = 270 - kFooterH - 8 - panelH;
    renderer.drawBox(8, panelY, 250, panelH);
    renderer.drawText("LOG", 12, panelY + 2, 1, kDim);
    int y = panelY + 11;
    const int first = static_cast<int>(events.size()) - lines;
    for (int i = 0; i < lines; ++i) {
        std::string line = events[static_cast<std::size_t>(first + i)];
        if (line.size() > 56) {
            line.resize(56);
        }
        renderer.drawText(line, 12, y, 1, kText);
        y += 8;
    }
}

}  // namespace neta
