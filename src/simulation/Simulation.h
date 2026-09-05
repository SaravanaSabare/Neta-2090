#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "core/Random.h"
#include "entities/Faction.h"
#include "entities/Npc.h"
#include "entities/Player.h"
#include "narrative/NarrativeState.h"
#include "narrative/OneShot.h"
#include "save/SaveSystem.h"
#include "world/World.h"

namespace neta::sim {

// Headless-capable world simulation. Knows nothing about SDL, windows, or
// pixels: it advances world state from (seed + tick count + inputs) and
// exposes read-only snapshots the renderer consumes. --headless exercises
// this class with zero graphics code linked into the path.
//
// One-shot layer: 3 seeded traces + 5 special NPC roles + 1 erase terminal.
// Same seed = same spots. Stages move only by player action (find / talk /
// erase), never by tick count.
class Simulation {
public:
    static constexpr double kTickHz = 10.0;
    static constexpr double kTickDt = 1.0 / kTickHz;
    static constexpr float kAreaW = 100.0f;
    static constexpr float kAreaH = 56.0f;
    static constexpr std::size_t kNpcCount = 12;
    static constexpr std::size_t kFactionCount = 4;
    static constexpr float kInteractRadius = 7.0f;

    struct TraceSpot {
        int id = 0;
        int district = 0;
        entities::Vec2 pos{};
        bool found = false;
    };

    struct EraseTerminal {
        int district = 0;
        entities::Vec2 pos{};
    };

    enum class InteractKind { None, Trace, Talk, Messenger, Place, EraseLocked, EraseWin };

    struct InteractResult {
        InteractKind kind = InteractKind::None;
        int index = -1;  // trace id or npc index
        std::string speaker;
        std::string text;
    };

    Simulation();

    // Builds the full placeholder world deterministically from the seed.
    void generate(std::uint64_t seed);

    // Advances the world by dt seconds (fixed step: kTickDt in-game).
    void tick(double dt);

    // Per-frame player intent from input; consumed by ticks. Queued rather
    // than applied immediately so sim results depend on ticks, not framerate.
    void setPlayerIntent(entities::Vec2 dir) { m_intent = dir; }

    std::uint64_t seed() const { return m_seed; }
    std::uint64_t tickCount() const { return m_tick; }
    double timeSeconds() const { return m_time; }

    const world::World& world() const { return m_world; }
    const entities::Player& player() const { return m_player; }
    const std::vector<entities::Npc>& npcs() const { return m_npcs; }
    const std::vector<entities::Faction>& factions() const { return m_factions; }
    const std::deque<std::string>& eventLog() const { return m_events; }
    const narrative::NarrativeState& narrative() const { return m_narrative; }

    // One-shot state.
    const std::vector<TraceSpot>& traces() const { return m_traces; }
    const EraseTerminal& eraseTerminal() const { return m_erase; }
    int tracesFound() const;
    bool won() const { return m_won; }
    bool messengerSpawned() const { return m_messengerSpawned; }
    oneshot::NpcRole npcRole(std::size_t i) const { return m_roles.at(i); }
    bool npcTalked(std::size_t i) const { return m_talked.at(i) != 0; }
    // Sector-local position of an npc (each district is a full 100x56 area).
    entities::Vec2 npcLocalPos(std::size_t i) const;
    // Ring travel: districts are sectors 0-4, east/west edges wrap around.
    int playerSector() const { return m_playerSector; }
    int visited(int sector) const { return m_visited.at(static_cast<std::size_t>(sector)); }
    // Test helper: move player instantly (clamped).
    void teleportPlayer(entities::Vec2 p) {
        m_player.setPosition(p);
        clampPlayer();
    }
    // Test helper: jump to a sector screen (marks visited like real travel).
    void travelTo(int sector, entities::Vec2 p) {
        if (sector >= 0 && sector < world::World::kDistrictCount) {
            m_playerSector = sector;
            onEnterSector();
        }
        m_player.setPosition(p);
        clampPlayer();
    }

    // Try E interaction at the player's current spot. Mutates found / talked
    // / messenger / won flags and returns the text the UI should show.
    InteractResult interact();

    save::SaveData snapshot() const;
    // Restores dynamic state; static content is rebuilt from the saved seed
    // so saves stay small and version-checkable. Returns false + error on
    // any inconsistency (wrong npc count, bad stage, ...).
    bool restore(const save::SaveData& data, std::string& error);

private:
    void buildStaticContent(std::uint64_t seed);
    void pushEvent(const std::string& event);
    void emitPlaceholderEvent();
    void clampPlayer();
    void onEnterSector();

    std::uint64_t m_seed = 0;
    std::uint64_t m_tick = 0;
    double m_time = 0.0;
    core::RngStream m_rng{1};  // the "sim" stream; state is saved/loaded
    world::World m_world;
    entities::Player m_player;
    std::vector<entities::Npc> m_npcs;
    std::vector<entities::Faction> m_factions;
    std::deque<std::string> m_events;
    entities::Vec2 m_intent{};
    narrative::NarrativeState m_narrative;
    // One-shot data: static spots rebuilt from seed, flags saved in v2.
    std::vector<TraceSpot> m_traces;
    EraseTerminal m_erase{};
    std::vector<oneshot::NpcRole> m_roles;
    std::vector<int> m_talked;
    bool m_messengerSpawned = false;
    bool m_won = false;
    // Ring position: which sector screen the player is on + visited marks.
    int m_playerSector = 0;
    std::vector<int> m_visited;
};

}  // namespace neta::sim
