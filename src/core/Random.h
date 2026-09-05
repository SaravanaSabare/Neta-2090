#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace neta::core {

// Deterministic RNG foundation for procedural generation.
//
// Contract: the same master seed + the same stream name + the same draw
// sequence ALWAYS produces the same values, on any machine, under the same
// game version. Systems must never share one global generator; each system
// derives its own stream (world, sim, npcs, ...) so that changing one
// generator does not reshuffle every other system's output.
//
// Implementation: SplitMix64. Small, fast, no global state, and the full
// stream state is a single u64, which makes save/load trivial.
class RngStream {
public:
    explicit RngStream(std::uint64_t seed);

    std::uint64_t nextU64();
    std::uint32_t nextU32();
    double nextDouble();  // uniform in [0, 1)
    int rangeInt(int lo, int hi);  // inclusive on both ends
    double rangeDouble(double lo, double hi);  // [lo, hi)
    std::size_t rangeIndex(std::size_t count);  // [0, count)

    std::uint64_t state() const { return m_state; }
    void setState(std::uint64_t state) { m_state = state; }

private:
    std::uint64_t m_state;
};

class Random {
public:
    explicit Random(std::uint64_t masterSeed)
        : m_master(masterSeed) {}

    std::uint64_t masterSeed() const { return m_master; }

    // Derive an independent stream from a numeric id.
    RngStream stream(std::uint64_t streamId) const;
    // Derive an independent stream from a human-readable name, e.g. "world".
    RngStream streamFor(const std::string& name) const;

    static std::uint64_t hashName(const std::string& name);  // FNV-1a
    static std::uint64_t mixSeed(std::uint64_t master, std::uint64_t id);

private:
    std::uint64_t m_master;
};

}  // namespace neta::core
