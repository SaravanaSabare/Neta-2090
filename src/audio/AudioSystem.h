#pragma once

namespace neta::audio {

// STUB. No audio device is opened in the foundation; SDL audio init and the
// retro sound engine (bleeps, ambient hum, event stingers) arrive with the
// audio milestone. The class exists so Game's lifecycle already has the
// init/update/shutdown slots and call sites don't churn later.
class AudioSystem {
public:
    bool init();
    void shutdown();
    void update();

private:
    bool m_ready = false;
};

}  // namespace neta::audio
