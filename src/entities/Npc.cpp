#include "entities/Npc.h"

namespace neta::entities {

namespace {
// Single-token actions only: save files are whitespace-delimited.
constexpr const char* kIdleActions[] = {"idle", "watching", "working", "seeking", "hiding"};
}  // namespace

Npc::Npc(EntityId id, std::string name)
    : Entity(id, std::move(name)) {}

void Npc::setLocal(float lx, float ly) {
    m_lx = lx < 0.0f ? 0.0f : (lx > 1.0f ? 1.0f : lx);
    m_ly = ly < 0.0f ? 0.0f : (ly > 1.0f ? 1.0f : ly);
}

void Npc::wander(core::RngStream& rng, float dt) {
    const float step = 0.25f * dt;
    setLocal(m_lx + static_cast<float>(rng.rangeDouble(-step, step)),
             m_ly + static_cast<float>(rng.rangeDouble(-step, step)));
    // Occasionally pick a new idle action (deterministic via the same stream).
    if (rng.rangeInt(0, 99) == 0) {
        m_currentAction = kIdleActions[rng.rangeIndex(5)];
    }
}

}  // namespace neta::entities
