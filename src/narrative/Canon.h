#pragma once

// FIXED CANON. These facts are authored and immutable: every playthrough,
// no matter how the procedural world varies, is built around them.
//
// Fixed (here) vs procedural (generated in world/simulation):
//   Fixed: start year 2090, the 2020s AI era, the five ancient AI corps,
//          the old AI crisis, Neta's rogue LLM and its time-crossing
//          application, the 3155 survivors, the objective ERASE THE
//          APPLICATION, and the rogue-AI manipulation strategy.
//   Procedural (NOT here): cities, factions, NPCs, relationships, secrets,
//          evidence placement, who knows what, who lies, and every path
//          the player might take toward the objective.
// Do not add generated content to this file.

namespace neta::canon {

inline constexpr int kStartYear = 2090;
inline constexpr const char* kAiEra = "2020s";
inline constexpr const char* kAncientCorps[] = {
    "Calculas AI", "RoC Inc.", "Henry Tech", "Neta", "Garuda AI"};
inline constexpr int kAncientCorpCount = 5;
inline constexpr int kLastStandYear = 3155;
inline constexpr const char* kObjective = "ERASE THE APPLICATION";

}  // namespace neta::canon
