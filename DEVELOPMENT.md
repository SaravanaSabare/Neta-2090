# DEVELOPMENT — architecture, contracts, roadmap

## 1. Architecture

```
                        GAME (core/Game)
                          |
            +-------------+--------------+
            |                            |
            v                            v
       SIMULATION                    RENDERING (+ UI)
  (owns truth, headless)          (reads truth, draws)
            |
  +---------+---------+-----------+
  |         |         |           |
  v         v         v           v
WORLD   ENTITIES   NARRATIVE    SAVE/LOAD
+Npcs   Player/    Canon +      (versioned
+Factions Npc/     Objective     snapshots)
       Faction      tracker
            |
            v
       AI (stub: AgentMind)
```

**The one rule:** state flows down. `Simulation` owns facts and consequences and
can run with no graphics (`--headless`). `Renderer`/`Ui` read the sim each frame
and never mutate it. `InputManager` reports keys; `Game` turns them into queued
player intent that fixed-step ticks consume. Future systems (factions acting,
NPC plans, event engine, LLM dialogue) plug into the left side; presentation
plugs into the right side.

## 2. Game loop

- Variable render rate; **fixed 10 Hz simulation step** with an accumulator.
- Max 5 catch-up ticks per frame; backlog is dropped after hitches (no spiral).
- `Clock` clamps deltas to 0.1 s (breakpoints / alt-tab safe).
- Player intent is queued per frame and consumed per tick, so results depend on
  tick count, not framerate.

## 3. Determinism contract

- `core::Random(masterSeed)` + `streamFor("system-name")` gives each system an
  independent SplitMix64 stream. Same seed + same stream + same draw order =
  identical numbers on any machine under the same game version.
- Never add a shared/global RNG. New systems derive a new named stream so
  existing worlds don't reshuffle.
- `Simulation::m_rng` (the `"sim"` stream) is part of the save format, so a
  loaded game continues bit-identically (proven by `neta_tests`).
- The startup log prints a `boot-stream` receipt: three numbers anyone can
  re-derive from the master seed.

## 4. Fixed canon vs procedural content

- `narrative/Canon.h` holds authored, immutable facts (2090, the five ancient
  corps, the application, 3155, the objective). It must never contain generated
  content.
- Everything else (districts, factions, NPCs, secrets, evidence, paths to the
  objective) is generated in `world/` + `simulation/` from the seed.
- This foundation's scripted beats (objective stages at ticks 10/30, the demo
  event feed) are **explicitly temporary**: real discoveries will drive
  `NarrativeState` once world simulation lands.

## 5. Save format + migration

- Text format, magic `NETASAVE <version>`, floats at `max_digits10` (bit-exact).
- `SaveData::kVersion` bumps on any layout change; `loadFromFile` rejects
  unknown versions with a clear error instead of reinterpreting bytes. Future
  versions add `if (version == N) migrate...` branches there.
- Location: `SDL_GetPrefPath` user dir, `./saves/` fallback. Never absolute.

## 6. Rendering (retro pipeline)

- Virtual 480×270 framebuffer via `SDL_RenderSetLogicalSize`, nearest-neighbor
  upscale (`SDL_HINT_RENDER_SCALE_QUALITY "0"`), optional scanline pass (F2).
- Text uses the built-in 3×5 `PixelFont` — no font-library dependency.
- Future pixel art / palettes / CRT shaders slot into `rendering/` only.

## 7. Steam seam

- All Steam access goes through `platform::ISteamProvider`. Development always
  uses `NullSteamProvider` (fully offline). Adding Steamworks later means one
  new provider class + construction in `Game` — no gameplay code changes, and
  no Steam headers may leak outside `platform/`.

## 8. Conventions

- C++20, `-Wall -Wextra -Wpedantic`, no warnings tolerated.
- Small classes, explicit ownership (`unique_ptr`), no singletons, no globals
  (save `Log`'s level flag).
- Dependencies stay minimal and justified: SDL2 only. No engine, no ECS lib,
  no JSON lib, no LLM SDK in the foundation.

## 9. Roadmap (later milestones, in order)

1. Procedural city + population generation (seeded districts/locations/NPCs).
2. Knowledge/secrets/relationships data model + event engine.
3. Faction agendas + NPC goal pursuit (attach `AgentMind` per agent).
4. Evidence/investigation + objective-path emergence; `NarrativeState` driven
   by discoveries (delete the scripted tick beats).
5. Terminal/dialogue UI, retro audio, Steamworks provider, achievements.
