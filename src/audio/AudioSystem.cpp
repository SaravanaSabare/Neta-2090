#include "audio/AudioSystem.h"

#include "core/Log.h"

namespace neta::audio {

bool AudioSystem::init() {
    m_ready = true;
    core::Log::info("audio", "audio stub ready (no device opened yet)");
    return true;
}

void AudioSystem::shutdown() {
    m_ready = false;
}

void AudioSystem::update() {
    // Future: pump music/SFX queues here.
}

}  // namespace neta::audio
