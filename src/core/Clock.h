#pragma once

#include <chrono>

namespace neta::core {

// Per-frame wall-clock timing. Keeps the game loop's delta time in one place
// so future systems (fixed-step accumulator, profiling) share one clock.
class Clock {
public:
    Clock()
        : m_last(std::chrono::steady_clock::now()) {}

    void tick() {
        const auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - m_last).count();
        m_last = now;
        // Clamp huge deltas (breakpoints, alt-tab) so the sim never spirals.
        if (dt < 0.0) {
            dt = 0.0;
        } else if (dt > 0.1) {
            dt = 0.1;
        }
        m_delta = dt;
        m_total += dt;
    }

    double deltaSeconds() const { return m_delta; }
    double totalSeconds() const { return m_total; }

private:
    std::chrono::steady_clock::time_point m_last;
    double m_delta = 0.0;
    double m_total = 0.0;
};

}  // namespace neta::core
