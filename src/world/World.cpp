#include "world/World.h"

#include "core/Random.h"

namespace neta::world {

namespace {
constexpr const char* kPrefixes[] = {"NEO", "OLD", "LOWER", "UPPER", "PORT", "GREY", "STATIC"};
constexpr const char* kCores[] = {"KOWLOON", "VERIDIA", "RUST", "CHROME", "HALIDE", "VANTINE", "NULLBAR"};
constexpr const char* kNotes[] = {
    "industrial sector", "market sprawl", "residential stacks", "data haven", "port authority",
    "night clinic row", "antenna fields"};

struct KindDef {
    const char* kind;
    const char* suffix;  // appended to district core for the place name
    const char* desc;
};
constexpr KindDef kKinds[] = {
    {"MARKET", "bazaar",
     "* STALLS SELLING REPAIRED TECH. EVERYONE HAGGLES. NO ONE ASKS WHERE IT CAME FROM."},
    {"CLINIC", "clinic",
     "* A NIGHT CLINIC. THE LIGHTS NEVER GO OFF. THE STAFF NEVER GIVE NAMES."},
    {"ANTENNA", "array",
     "* ANTENNA FIELDS HUMMING ALL NIGHT. SOME CHANNELS SHOULD NOT EXIST."},
    {"PORT GATE", "gate",
     "* PORT AUTHORITY GATE. MANIFESTS ARE CHECKED. MANIFESTS ARE ALSO FOR SALE."},
    {"DATA HAVEN", "haven",
     "* A DATA HAVEN. OLD DRIVES STACKED TO THE CEILING. SOMEONE READS THEM ALL."},
    {"CHAPEL", "chapel",
     "* A SMALL CHAPEL OF THE SIGNAL FAITHFUL. CANDLES. WHISPERS. NEW INK ON OLD PRAYERS."},
    {"DEPOT", "depot",
     "* A FREIGHT DEPOT. CONTAINERS COME AND GO AT HOURS NO HONEST TRADE KEEPS."},
    {"ROOFTOP", "roof",
     "* A ROOFTOP GARDEN ABOVE THE SMOG. FROM HERE THE WHOLE DISTRICT LOOKS QUIET."},
};
constexpr std::size_t kKindCount = 8;
}  // namespace

void World::generate(std::uint64_t seed) {
    m_districts.clear();
    m_locations.clear();
    core::Random rng(seed);
    // District names first, in the exact original draw order, so old seeds
    // keep their names. Wealth/bustle are appended after per district.
    core::RngStream s = rng.streamFor("world");
    for (int i = 0; i < kDistrictCount; ++i) {
        District d;
        d.index = i;
        d.name = std::string(kPrefixes[s.rangeIndex(7)]) + "-" + kCores[s.rangeIndex(7)];
        d.note = kNotes[s.rangeIndex(7)];
        d.wealth = s.rangeInt(5, 95);
        d.bustle = s.rangeInt(5, 95);
        m_districts.push_back(std::move(d));
    }
    // Places on their own stream: 3-5 per district, kinds shuffled.
    core::RngStream c = rng.streamFor("city");
    const float colW = kAreaW / static_cast<float>(kDistrictCount);
    int nextId = 0;
    for (int di = 0; di < kDistrictCount; ++di) {
        const int count = 3 + static_cast<int>(c.rangeIndex(3));  // 3-5
        // Shuffle kind order per district so neighbors differ.
        std::size_t order[kKindCount];
        for (std::size_t k = 0; k < kKindCount; ++k) {
            order[k] = k;
        }
        for (std::size_t k = kKindCount - 1; k > 0; --k) {
            const std::size_t j = c.rangeIndex(k + 1);
            const std::size_t tmp = order[k];
            order[k] = order[j];
            order[j] = tmp;
        }
        // District core word for place names (part after the dash).
        std::string core = m_districts[static_cast<std::size_t>(di)].name;
        const std::size_t dash = core.find('-');
        if (dash != std::string::npos) {
            core = core.substr(dash + 1);
        }
        for (int li = 0; li < count; ++li) {
            const KindDef& kd = kKinds[order[static_cast<std::size_t>(li) % kKindCount]];
            Location loc;
            loc.id = nextId++;
            loc.district = di;
            loc.kind = kd.kind;
            loc.name = core + " " + kd.suffix;
            loc.desc = kd.desc;
            const double fx = 0.15 + c.nextDouble() * 0.7;
            const double fy = 0.15 + c.nextDouble() * 0.7;
            loc.x = (static_cast<float>(di) + static_cast<float>(fx)) * colW;
            loc.y = 6.0f + static_cast<float>(fy) * (kAreaH - 12.0f);
            m_locations.push_back(std::move(loc));
        }
    }
}

}  // namespace neta::world
