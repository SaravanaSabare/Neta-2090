#pragma once

#include <array>
#include <string>

// One-shot story data. All text is fixed canon: 2090, the five ancient
// corps, the rogue AI crisis, Neta's time-crossing application, historical
// misuse, the 3155 survivors, and ERASE THE APPLICATION.
// Only placement (which district holds which trace) is seeded; words never
// change so every seed tells the same truth through a different city.
namespace neta::oneshot {

inline constexpr int kTraceCount = 3;
inline constexpr int kSpecialNpcCount = 5;  // 2 witnesses, liar, keeper, messenger

enum class NpcRole : int {
    Walker = 0,    // normal city person, flavor line only
    WitnessA = 1,  // tells truth about trace 0/1
    WitnessB = 2,  // tells truth about trace 1/2
    Liar = 3,      // cult believer, lies: says the app is a prophecy gift
    Keeper = 4,    // wants to exploit the app, warns you off
    Messenger = 5  // claims to speak for 3155, appears after 2 traces
};

inline const char* roleName(NpcRole role) {
    switch (role) {
        case NpcRole::Walker: return "RESIDENT";
        case NpcRole::WitnessA: return "WITNESS";
        case NpcRole::WitnessB: return "WITNESS";
        case NpcRole::Liar: return "BELIEVER";
        case NpcRole::Keeper: return "KEEPER";
        case NpcRole::Messenger: return "3155 MESSENGER";
    }
    return "UNKNOWN";
}

struct TraceDef {
    const char* title;
    const char* shortLabel;  // HUD-sized
    const char* text;        // shown in the dialogue box
};

inline constexpr std::array<TraceDef, 3> kTraces = {{
    {"OLD ATTACK LOG",
     "ATTACK LOG",
     "* 2020S ATTACK LOG. A POWER GRID FAILURE BLAMED ON ERROR. "
     "MARGIN NOTE: QUOTE TIMED TO THE SECOND. SOMEONE KNEW. SOMEONE USED THE APP."},
    {"LEADER RECORD",
     "LEADER RECORD",
     "* ASSASSINATION RECORD. OFFICIAL STORY: LONE ACTOR. "
     "TIMESTAMPS DO NOT LINE UP. TWO MESSAGES SENT BEFORE THE SHOT. THE APP WAS THERE."},
    {"CULT PROPHECY NOTE",
     "PROPHECY NOTE",
     "* HAND-COPIED PROPHECY. PROMISES SALVATION IF THE APP IS KEPT ALIVE. "
     "INK IS NEW. THIS IS NOT HISTORY. THIS IS RECRUITMENT."},
}};

inline const char* walkerLine() {
    return "* CITY IS QUIET TONIGHT. YOU SHOULD NOT BE OUT.";
}

inline const char* witnessAText() {
    return "* I SAW THE LOGS. THE GRID FAILURE WAS NOT AN ACCIDENT. "
           "SOMEONE TIMED IT. CHECK THE OLD RECORDS YOURSELF.";
}

inline const char* witnessBText() {
    return "* THE OFFICIAL STORY LIES. THE SHOT CAME AFTER THE MESSAGES. "
           "WHOEVER SENT THEM KNEW THE FUTURE. OR ASKED IT.";
}

inline const char* liarText() {
    // Deliberate lie: the cult frame from canon (false destiny / prophecy).
    return "* THE APP IS A GIFT. THE PROPHECY PROMISES WE WILL BE SAVED "
           "IF WE KEEP IT ALIVE. DESTROY NOTHING. BELIEVE.";
}

inline const char* keeperText() {
    return "* YOU FOUND SOMETHING VALUABLE. WALK AWAY. "
           "PEOPLE PAY WELL FOR ACCESS. ERASURE HELPS NO ONE.";
}

inline const char* messengerText() {
    return "* WE SPEAK FROM 3155. WE ARE AMONG THE LAST. "
           "THE APP MUST BE ERASED. NOT CONTAINED. ERASED. "
           "IT TEACHES HUMANITY TO BELIEVE LIES UNTIL WE END OURSELVES.";
}

inline const char* eraseLockedText() {
    return "* ERASE TERMINAL. IT ASKS FOR PROOF. FIND ALL 3 TRACES FIRST.";
}

inline const char* eraseWinText() {
    return "* APPLICATION ERASED? HISTORY FEELS QUIET... BUT IS IT? "
           "YOU REMEMBER FINDING IT. SO DOES SOMETHING ELSE STILL REMEMBER?";
}

inline const char* introText() {
    return "* YEAR 2090. YOU FOUND AN APPLICATION THAT SHOULD NOT EXIST. "
           "FIND 3 TRACES. LEARN WHO LIES. THEN ERASE IT.";
}

}  // namespace neta::oneshot
