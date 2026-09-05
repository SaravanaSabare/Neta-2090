#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace neta::save {

// Versioned save data. kVersion MUST be bumped whenever this layout changes,
// and loadFromFile must keep (or explicitly migrate) every older version.
// See SaveSystem::loadFromFile for the migration policy.
struct NpcSave {
    std::uint32_t id = 0;
    int faction = -1;
    int district = 0;
    float lx = 0.5f;
    float ly = 0.5f;
    std::string action = "idle";
};

struct SaveData {
    static constexpr std::uint32_t kVersion = 3;

    std::uint64_t seed = 0;
    std::uint64_t tick = 0;
    std::uint64_t rngState = 0;  // RngStream state: sim continues identically
    double time = 0.0;
    float playerX = 0.0f;
    float playerY = 0.0f;
    int stage = 0;  // ObjectiveStage as int
    std::vector<NpcSave> npcs;
    // One-shot progress (v2). Static spots are rebuilt from the seed, so
    // only these small flags are stored. v1 saves load with all zeros.
    std::vector<int> traces;  // size 3, 1 = found
    std::vector<int> talked;  // size N, 1 = talked to this npc index
    int messengerSpawned = 0;
    int won = 0;
    // Ring position (v3). Static spots rebuild from seed; only these ride along.
    int playerSector = 0;
    std::vector<int> visited;  // size 5, 1 = sector entered
};

bool snapshotsEqual(const SaveData& a, const SaveData& b, float eps = 1e-4f);

class SaveSystem {
public:
    // Never hard-codes an absolute location: prefers the OS per-user dir
    // (SDL_GetPrefPath) and falls back to ./saves. Creates dirs on save.
    static std::string defaultSavePath(const std::string& filename);
    static bool saveToFile(const std::string& path, const SaveData& data);
    static bool loadFromFile(const std::string& path, SaveData& out, std::string& error);
};

}  // namespace neta::save
