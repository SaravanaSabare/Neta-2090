#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace neta::world {

// One city district. Placeholder for the future procedural city generator
// (geometry, locations, ownership, evidence placement will extend this).
struct District {
    int index = 0;
    std::string name;
    std::string note;
};

class World {
public:
    static constexpr int kDistrictCount = 5;

    // Deterministic: same seed always yields the same districts.
    void generate(std::uint64_t seed);

    const std::vector<District>& districts() const { return m_districts; }
    int districtCount() const { return static_cast<int>(m_districts.size()); }
    const District& district(int index) const { return m_districts.at(index); }

private:
    std::vector<District> m_districts;
};

}  // namespace neta::world
