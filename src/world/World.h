#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace neta::world {

// One city district. Name + note draws come first on the "world" stream so
// they never reshuffle; wealth/bustle are appended after and are flavor only.
struct District {
    int index = 0;
    std::string name;
    std::string note;
    int wealth = 50;  // 0-100, flavor
    int bustle = 50;  // 0-100, flavor
};

// A named place inside a district. Static: rebuilt from the seed on the
// "city" stream, so saves stay small. Position is in world units (0-100 x,
// 0-56 y), the same space the player walks in.
struct Location {
    int id = 0;
    int district = 0;
    std::string kind;
    std::string name;
    std::string desc;
    float x = 0.0f;
    float y = 0.0f;
};

class World {
public:
    static constexpr int kDistrictCount = 5;
    static constexpr float kAreaW = 100.0f;
    static constexpr float kAreaH = 56.0f;

    // Deterministic: same seed always yields the same districts + places.
    void generate(std::uint64_t seed);

    const std::vector<District>& districts() const { return m_districts; }
    int districtCount() const { return static_cast<int>(m_districts.size()); }
    const District& district(int index) const { return m_districts.at(index); }

    const std::vector<Location>& locations() const { return m_locations; }
    const Location& location(int id) const { return m_locations.at(static_cast<std::size_t>(id)); }

private:
    std::vector<District> m_districts;
    std::vector<Location> m_locations;
};

}  // namespace neta::world
