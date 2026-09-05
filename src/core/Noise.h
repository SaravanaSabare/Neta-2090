#pragma once

#include <cmath>
#include <cstdint>

// Seeded 2D gradient noise ("uber noise" core, 2D edition).
//
// Why this file exists: biome ground texture, building-silhouette variation,
// and flat-ground placement checks all read from here. Same seed + same
// coords = same value on any machine, with zero global state and zero RNG
// streams touched (so saves, ticks, and determinism tests are unaffected).
//
// Pieces:
//   noise2  - one octave of gradient noise in [-1, 1].
//   fbm     - fractal stack of octaves (persistence scales height down,
//             lacunarity scales frequency up per octave).
//   ridged  - sharp-crested variant (1 - |noise|) for antenna streaks.
//   warped  - domain warp: a low-frequency sample bends the main sample
//             point first, killing grid repetition for organic curves.
//   isFlat  - finite-difference slope check for structure placement: the
//             steepest of 4 neighbor samples must stay under slopeLimit.
//             (Analytic derivatives are the textbook 3D-terrain way; for
//             placing rects on a 2D map this decides identically.)
//
// Tuning: one Preset per biome. Octaves ~3-5, persistence ~0.5,
// lacunarity ~2.0, warp 0 (off) to ~1.5 (wild), scale = feature size in
// world units (bigger = broader shapes).
namespace neta::noise {

struct Preset {
    int octaves = 4;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float warp = 0.6f;
    float scale = 18.0f;
};

inline std::uint64_t hashCell(std::uint64_t seed, int x, int y) {
    // SplitMix64 over a mixed key. Stateless and platform-stable.
    std::uint64_t z = seed + static_cast<std::uint64_t>(x) * 0x9e3779b97f4a7c15ULL +
                      static_cast<std::uint64_t>(y) * 0xbf58476d1ce4e5b9ULL;
    z += 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

inline float noise2(std::uint64_t seed, float fx, float fy) {
    const int xi = static_cast<int>(std::floor(fx));
    const int yi = static_cast<int>(std::floor(fy));
    const float xf = fx - static_cast<float>(xi);
    const float yf = fy - static_cast<float>(yi);
    // 8 gradient directions; hash picks one per corner.
    auto grad = [](std::uint64_t h, float x, float y) {
        switch (h & 7ULL) {
            case 0: return x + y;
            case 1: return x - y;
            case 2: return -x + y;
            case 3: return -x - y;
            case 4: return x;
            case 5: return -x;
            case 6: return y;
            default: return -y;
        }
    };
    const float u = xf * xf * (3.0f - 2.0f * xf);
    const float v = yf * yf * (3.0f - 2.0f * yf);
    const float a = grad(hashCell(seed, xi, yi), xf, yf);
    const float b = grad(hashCell(seed, xi + 1, yi), xf - 1.0f, yf);
    const float c = grad(hashCell(seed, xi, yi + 1), xf, yf - 1.0f);
    const float d = grad(hashCell(seed, xi + 1, yi + 1), xf - 1.0f, yf - 1.0f);
    // Gradient dots land in roughly [-1, 1]; normalize by ~1.42 to fit.
    return ((a + (b - a) * u) + ((c + (d - c) * u) - (a + (b - a) * u)) * v) * 0.7071f;
}

inline float fbm(std::uint64_t seed, float fx, float fy, const Preset& p) {
    float amp = 0.5f;
    float freq = 1.0f / p.scale;
    float sum = 0.0f;
    float norm = 0.0f;
    for (int o = 0; o < p.octaves; ++o) {
        sum += noise2(seed + static_cast<std::uint64_t>(o) * 0x9e3779b97f4a7c15ULL, fx * freq,
                      fy * freq) *
               amp;
        norm += amp;
        amp *= p.persistence;
        freq *= p.lacunarity;
    }
    return norm > 0.0f ? sum / norm : 0.0f;
}

inline float ridged(std::uint64_t seed, float fx, float fy, const Preset& p) {
    // Sharp crests instead of soft blobs: 1 - |fbm|.
    const float v = fbm(seed, fx, fy, p);
    return 1.0f - (v < 0.0f ? -v : v);
}

inline float warped(std::uint64_t seed, float fx, float fy, const Preset& p) {
    if (p.warp <= 0.0f) {
        return fbm(seed, fx, fy, p);
    }
    // Low-frequency bend, then the main sample at the bent point.
    const Preset bend{2, 0.5f, 2.0f, 0.0f, p.scale * 3.0f};
    const float qx = fbm(seed ^ 0x12345678ULL, fx, fy, bend);
    const float qy = fbm(seed ^ 0x87654321ULL, fx, fy, bend);
    return fbm(seed, fx + qx * p.warp * p.scale * 0.25f, fy + qy * p.warp * p.scale * 0.25f, p);
}

inline bool isFlat(std::uint64_t seed, float fx, float fy, const Preset& p, float step,
                   float slopeLimit) {
    // Steepest neighbor difference per world unit must stay under the limit.
    const float c = warped(seed, fx, fy, p);
    float worst = 0.0f;
    const float samples[4] = {
        warped(seed, fx + step, fy, p), warped(seed, fx - step, fy, p),
        warped(seed, fx, fy + step, p), warped(seed, fx, fy - step, p)};
    for (float s : samples) {
        float d = s - c;
        if (d < 0.0f) {
            d = -d;
        }
        d /= step;
        if (d > worst) {
            worst = d;
        }
    }
    return worst <= slopeLimit;
}

}  // namespace neta::noise
