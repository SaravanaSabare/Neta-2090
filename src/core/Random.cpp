#include "core/Random.h"

namespace neta::core {

RngStream::RngStream(std::uint64_t seed)
    : m_state(seed != 0 ? seed : 0x9E3779B97F4A7C15ULL) {}

std::uint64_t RngStream::nextU64() {
    std::uint64_t z = (m_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

std::uint32_t RngStream::nextU32() {
    return static_cast<std::uint32_t>(nextU64() >> 32);
}

double RngStream::nextDouble() {
    // Take the top 53 bits for a uniform IEEE-754 mantissa.
    return static_cast<double>(nextU64() >> 11) * (1.0 / 9007199254740992.0);
}

int RngStream::rangeInt(int lo, int hi) {
    if (hi <= lo) {
        return lo;
    }
    const auto span = static_cast<std::uint64_t>(hi - lo) + 1ULL;
    return lo + static_cast<int>(nextU64() % span);
}

double RngStream::rangeDouble(double lo, double hi) {
    if (hi <= lo) {
        return lo;
    }
    return lo + nextDouble() * (hi - lo);
}

std::size_t RngStream::rangeIndex(std::size_t count) {
    if (count == 0) {
        return 0;
    }
    return static_cast<std::size_t>(nextU64() % count);
}

RngStream Random::stream(std::uint64_t streamId) const {
    return RngStream{mixSeed(m_master, streamId)};
}

RngStream Random::streamFor(const std::string& name) const {
    return stream(hashName(name));
}

std::uint64_t Random::hashName(const std::string& name) {
    // FNV-1a 64-bit.
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : name) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::uint64_t Random::mixSeed(std::uint64_t master, std::uint64_t id) {
    std::uint64_t z = master + id * 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);
    return z != 0 ? z : 0x9E3779B97F4A7C15ULL;
}

}  // namespace neta::core
