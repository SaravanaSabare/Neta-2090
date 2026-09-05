#pragma once

#include "core/Log.h"

namespace neta::platform {

// Steam integration point. Steamworks is NOT linked in the foundation, but
// every future call site must go through ISteamProvider so the swap-in is
// mechanical: implement SteamworksProvider, construct it in Game when a
// Steam build is detected, and the rest of the game never changes.
//
// Rules: the game must always run offline (NullSteamProvider), achievements
// and stats must be no-ops without Steam, and no gameplay code may include
// Steam headers directly.
class ISteamProvider {
public:
    virtual ~ISteamProvider() = default;
    virtual const char* name() const = 0;
    // Returns true only when a real Steam client is bound.
    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual bool overlayAvailable() const { return false; }
    virtual void unlockAchievement(const char* /*achievementId*/) {}
    virtual void setStat(const char* /*name*/, int /*value*/) {}
};

class NullSteamProvider : public ISteamProvider {
public:
    const char* name() const override { return "NullSteam (offline dev)"; }
    bool init() override {
        core::Log::info("steam", "Steam disabled: running offline ({})", name());
        return false;
    }
    void shutdown() override {}
};

}  // namespace neta::platform
