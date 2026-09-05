#pragma once

namespace neta::narrative {

// Tracks how much of the fixed objective the player has uncovered.
// Placeholder state machine: later, stages will be advanced by discoveries
// in the simulated world (evidence, NPC testimony), never by tick counts.
enum class ObjectiveStage : int {
    Undiscovered = 0,  // player has not found the application yet
    Aware = 1,  // player knows the application exists
    Investigating = 2,  // player is tracing its uses through history
    Contact3155 = 3,  // player has heard the survivors' warning
    ErasureKnown = 4,  // player knows it must be erased
};

class NarrativeState {
public:
    void reset();
    void setStage(ObjectiveStage stage) { m_stage = stage; }
    ObjectiveStage stage() const { return m_stage; }

    static const char* stageName(ObjectiveStage stage);

private:
    ObjectiveStage m_stage = ObjectiveStage::Undiscovered;
};

}  // namespace neta::narrative
