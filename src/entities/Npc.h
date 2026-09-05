#pragma once

#include <string>

#include "core/Random.h"
#include "entities/Entity.h"

namespace neta::entities {

// Placeholder NPC. The fields below are the minimum hooks the future agent
// model needs (identity via Entity, faction, location, current action).
// Goals, knowledge, beliefs, relationships, and plans will live in ai/
// and narrative/ systems and operate ON these entities; see AgentMind.
class Npc : public Entity {
public:
    Npc(EntityId id, std::string name);

    int factionId() const { return m_factionId; }
    void setFactionId(int factionId) { m_factionId = factionId; }

    int district() const { return m_district; }
    void setDistrict(int district) { m_district = district; }

    // Position inside the NPC's district, normalized to [0, 1]. The renderer
    // maps (district, lx, ly) to screen space.
    float localX() const { return m_lx; }
    float localY() const { return m_ly; }
    void setLocal(float lx, float ly);

    const std::string& currentAction() const { return m_currentAction; }
    void setCurrentAction(const std::string& action) { m_currentAction = action; }

    // Deterministic idle behavior: small random walk. Uses the simulation's
    // event stream, so identical seeds + tick counts give identical motion.
    void wander(core::RngStream& rng, float dt);

private:
    int m_factionId = -1;
    int m_district = 0;
    float m_lx = 0.5f;
    float m_ly = 0.5f;
    std::string m_currentAction = "idle";
};

}  // namespace neta::entities
