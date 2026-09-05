#include "narrative/NarrativeState.h"

namespace neta::narrative {

void NarrativeState::reset() {
    m_stage = ObjectiveStage::Undiscovered;
}

const char* NarrativeState::stageName(ObjectiveStage stage) {
    switch (stage) {
        case ObjectiveStage::Undiscovered: return "UNDISCOVERED";
        case ObjectiveStage::Aware: return "AWARE";
        case ObjectiveStage::Investigating: return "INVESTIGATING";
        case ObjectiveStage::Contact3155: return "CONTACT-3155";
        case ObjectiveStage::ErasureKnown: return "ERASURE-KNOWN";
    }
    return "UNKNOWN";
}

}  // namespace neta::narrative
