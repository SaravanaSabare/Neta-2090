#pragma once

#include "entities/Entity.h"

namespace neta::ai {

// STUB MODULE. Advanced NPC AI (goals, planning, knowledge, beliefs,
// relationships, deception) is intentionally NOT implemented in the
// foundation. This class reserves the module's place in the architecture
// and documents the contract the future AI must obey:
//
//   - The Simulation owns truth: facts, state, resources, consequences.
//   - Minds only decide what an agent TRIES to do; the sim resolves it.
//   - Minds must be deterministic given (seed, world state, tick).
//   - A future LLM layer may propose dialogue/plans, but the sim validates
//     every effect before it becomes canon. The LLM never owns game state.
//
// When agent AI lands, one AgentMind will attach to each Npc/Faction and be
// ticked by Simulation; rendering and UI will only ever read the results.
class AgentMind {
public:
    explicit AgentMind(entities::EntityId ownerId)
        : m_ownerId(ownerId) {}

    entities::EntityId ownerId() const { return m_ownerId; }

    // Placeholder tick: does nothing by design.
    void updatePlaceholder() {}

private:
    entities::EntityId m_ownerId;
};

}  // namespace neta::ai
