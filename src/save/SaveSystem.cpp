#include "save/SaveSystem.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

#include <SDL.h>

#include "core/Log.h"

namespace neta::save {

namespace {
constexpr const char* kMagic = "NETASAVE";
}

bool snapshotsEqual(const SaveData& a, const SaveData& b, float eps) {
    if (a.seed != b.seed || a.tick != b.tick || a.rngState != b.rngState || a.stage != b.stage) {
        return false;
    }
    if (std::fabs(a.time - b.time) > eps) {
        return false;
    }
    if (std::fabs(a.playerX - b.playerX) > eps || std::fabs(a.playerY - b.playerY) > eps) {
        return false;
    }
    if (a.npcs.size() != b.npcs.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.npcs.size(); ++i) {
        const NpcSave& x = a.npcs[i];
        const NpcSave& y = b.npcs[i];
        if (x.id != y.id || x.faction != y.faction || x.district != y.district || x.action != y.action) {
            return false;
        }
        if (std::fabs(x.lx - y.lx) > eps || std::fabs(x.ly - y.ly) > eps) {
            return false;
        }
    }
    if (a.traces != b.traces || a.talked != b.talked) {
        return false;
    }
    if (a.messengerSpawned != b.messengerSpawned || a.won != b.won) {
        return false;
    }
    return true;
}

std::string SaveSystem::defaultSavePath(const std::string& filename) {
    std::filesystem::path dir;
    char* pref = SDL_GetPrefPath("neta", "neta-foundation");
    if (pref != nullptr) {
        dir = std::filesystem::path(pref) / "saves";
        SDL_free(pref);
    } else {
        dir = std::filesystem::path("saves");
    }
    return (dir / filename).string();
}

bool SaveSystem::saveToFile(const std::string& path, const SaveData& data) {
    std::error_code ec;
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            core::Log::error("save", "cannot create dir '{}': {}", parent.string(), ec.message());
            return false;
        }
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        core::Log::error("save", "cannot open '{}' for writing", path);
        return false;
    }
    out << kMagic << ' ' << SaveData::kVersion << '\n';
    out << "seed " << data.seed << '\n';
    out << "tick " << data.tick << '\n';
    out << "rng " << data.rngState << '\n';
    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    out << "time " << data.time << '\n';
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "player " << data.playerX << ' ' << data.playerY << '\n';
    out << "stage " << data.stage << '\n';
    out << "npcs " << data.npcs.size() << '\n';
    for (const NpcSave& n : data.npcs) {
        out << "npc " << n.id << ' ' << n.faction << ' ' << n.district << ' ' << n.lx << ' ' << n.ly
            << ' ' << n.action << '\n';
    }
    // v2 one-shot progress. Fixed-size small lines.
    out << "traces";
    for (int v : data.traces) {
        out << ' ' << v;
    }
    out << '\n';
    out << "talked";
    for (int v : data.talked) {
        out << ' ' << v;
    }
    out << '\n';
    out << "oneshot " << data.messengerSpawned << ' ' << data.won << '\n';
    out.flush();
    if (!out) {
        core::Log::error("save", "failed while writing '{}'", path);
        return false;
    }
    core::Log::info("save", "saved tick {} to '{}'", data.tick, path);
    return true;
}

bool SaveSystem::loadFromFile(const std::string& path, SaveData& out, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot open save file '" + path + "'";
        return false;
    }
    std::string magic;
    std::uint32_t version = 0;
    if (!(in >> magic >> version) || magic != kMagic) {
        error = "not a NETA save file: '" + path + "'";
        return false;
    }
    // Migration policy: v1 had no one-shot lines (defaults to zeros).
    // v2 adds traces / talked / oneshot. Never silently reinterpret bytes.
    if (version != 1 && version != SaveData::kVersion) {
        error = "unsupported save version " + std::to_string(version) + " (game supports " +
                std::to_string(SaveData::kVersion) + ")";
        return false;
    }

    SaveData data;
    std::string key;
    std::size_t npcCount = 0;
    // Fixed-order, human-readable lines. Written with max_digits10 precision
    // so float round-trips are bit-exact.
    if (!(in >> key >> data.seed) || key != "seed") {
        error = "corrupt save: expected 'seed'";
        return false;
    }
    if (!(in >> key >> data.tick) || key != "tick") {
        error = "corrupt save: expected 'tick'";
        return false;
    }
    if (!(in >> key >> data.rngState) || key != "rng") {
        error = "corrupt save: expected 'rng'";
        return false;
    }
    if (!(in >> key >> data.time) || key != "time") {
        error = "corrupt save: expected 'time'";
        return false;
    }
    if (!(in >> key >> data.playerX >> data.playerY) || key != "player") {
        error = "corrupt save: expected 'player'";
        return false;
    }
    if (!(in >> key >> data.stage) || key != "stage") {
        error = "corrupt save: expected 'stage'";
        return false;
    }
    if (!(in >> key >> npcCount) || key != "npcs") {
        error = "corrupt save: expected 'npcs'";
        return false;
    }
    data.npcs.reserve(npcCount);
    for (std::size_t i = 0; i < npcCount; ++i) {
        NpcSave n;
        if (!(in >> key >> n.id >> n.faction >> n.district >> n.lx >> n.ly >> n.action) ||
            key != "npc") {
            error = "corrupt save: bad npc row " + std::to_string(i);
            return false;
        }
        data.npcs.push_back(std::move(n));
    }
    // v1 files end here: migrate to zeros.
    data.traces = {0, 0, 0};
    data.talked.assign(npcCount, 0);
    data.messengerSpawned = 0;
    data.won = 0;
    if (version == 1) {
        out = std::move(data);
        core::Log::info("save", "loaded v1 tick {} from '{}' (migrated)", out.tick, path);
        return true;
    }
    // v2 lines: traces / talked / oneshot. Line-oriented so a missing tail
    // is a clear error, not silent zeros.
    std::string line;
    std::getline(in, line);  // consume end of last npc line
    if (!std::getline(in, line) || line.rfind("traces", 0) != 0) {
        error = "corrupt save: expected 'traces'";
        return false;
    }
    {
        std::istringstream ls(line);
        std::string k;
        ls >> k;
        data.traces.clear();
        int v = 0;
        while (ls >> v) {
            data.traces.push_back(v != 0 ? 1 : 0);
        }
        if (data.traces.size() != 3) {
            error = "corrupt save: bad traces line";
            return false;
        }
    }
    if (!std::getline(in, line) || line.rfind("talked", 0) != 0) {
        error = "corrupt save: expected 'talked'";
        return false;
    }
    {
        std::istringstream ls(line);
        std::string k;
        ls >> k;
        data.talked.clear();
        int v = 0;
        while (ls >> v) {
            data.talked.push_back(v != 0 ? 1 : 0);
        }
        if (data.talked.size() != npcCount) {
            error = "corrupt save: bad talked line";
            return false;
        }
    }
    if (!std::getline(in, line) || line.rfind("oneshot", 0) != 0) {
        error = "corrupt save: expected 'oneshot'";
        return false;
    }
    {
        std::istringstream ls(line);
        std::string k;
        int m = 0;
        int w = 0;
        if (!(ls >> k >> m >> w) || k != "oneshot") {
            error = "corrupt save: bad oneshot line";
            return false;
        }
        data.messengerSpawned = (m != 0) ? 1 : 0;
        data.won = (w != 0) ? 1 : 0;
    }
    out = std::move(data);
    core::Log::info("save", "loaded tick {} from '{}'", out.tick, path);
    return true;
}

}  // namespace neta::save
