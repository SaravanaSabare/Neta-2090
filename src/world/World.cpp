#include "world/World.h"

#include "core/Random.h"

namespace neta::world {

namespace {
// Placeholder name pools. A future generator will build far richer,
// seeded city layouts; the contract (seed -> same city) already holds.
constexpr const char* kPrefixes[] = {"NEO", "OLD", "LOWER", "UPPER", "PORT", "GREY", "STATIC"};
constexpr const char* kCores[] = {"KOWLOON", "VERIDIA", "RUST", "CHROME", "HALIDE", "VANTINE", "NULLBAR"};
constexpr const char* kNotes[] = {
    "industrial sector", "market sprawl", "residential stacks", "data haven", "port authority",
    "night clinic row", "antenna fields"};
}  // namespace

void World::generate(std::uint64_t seed) {
    m_districts.clear();
    core::Random rng(seed);
    core::RngStream s = rng.streamFor("world");
    for (int i = 0; i < kDistrictCount; ++i) {
        District d;
        d.index = i;
        d.name = std::string(kPrefixes[s.rangeIndex(7)]) + "-" + kCores[s.rangeIndex(7)];
        d.note = kNotes[s.rangeIndex(7)];
        m_districts.push_back(std::move(d));
    }
}

}  // namespace neta::world
