# HANDOFF — what a future AI session needs to pick up NETA-2090

> Keep this file updated at the end of every work session.
> The user prefers SIMPLE, non-technical language. Short answers. No jargon walls.

## 1. What this game is

Retro cyberpunk procedural narrative/simulation game, C++20 + SDL2 only, Windows.
Working title: **NETA-2090**. Repo: https://github.com/SaravanaSabare/Neta-2090 (public, branch `main`).

**Story in one line:** Year 2090, you find an app that crosses time; people from 3155 say it must be ERASED.

**Canon lives in:** `src/narrative/Canon.h` (fixed facts) + `src/narrative/OneShot.h` (one-shot texts).
**Rule:** fixed facts never change; city/factions/NPCs/secrets/who-lies are seeded per world.

## 2. How to play right now (one-shot, ~7 min)

Title (E) → explore 5 biome sectors in an east/west ring → find 3 traces (E) →
talk to NPCs (witnesses truth, believer lies, keeper tempts, walkers hint in
SECTOR coordinates) → messenger from 3155 appears after 2 traces → erase door
with 3/3 → win screen. No losing yet.

Controls: WASD/arrows move (walk off edge = travel), E/Z/Enter talk+advance text,
Space pause, F1 log, F2 scanlines, F5 save, F9 load, Esc quit.

## 3. Architecture (do not break these)

- `Simulation` owns ALL truth, runs headless (`--headless`). Renderer/UI read only.
- Fixed 10 Hz sim step, accumulator, max 5 catch-up ticks. Player intent queued per frame.
- Determinism: each system gets its own RNG stream — `world`, `sim`, `oneshot`,
  `city`, `pop`, `turf`. New systems MUST add a new stream, never reuse draws.
  `src/core/Noise.h` is stateless hash noise (no streams touched).
- Rendering: 480x270 virtual framebuffer, built-in 3x5 PixelFont, Undertale style
  (black boxes, white borders, `*` text, typewriter, red-heart player).
- C++20, `-Wall -Wextra -Wpedantic` clean. SDL2 only, no new deps.
- Saves: text format `NETASAVE <version>`, currently **v3**
  (v1: base, v2: traces/talked/oneshot, v3: sector/visited). Static content
  rebuilds from seed; only dynamic flags are stored. Old versions must keep loading.

## 4. Current world model

- 5 districts = ring sectors 0-4 (east/west wrap). One full 100x56 screen each.
- Per sector: theme biome art + noise ground, 3-5 named Location(s) (examine w/ E),
  faction turf, 3-6 future building slots (NOT built yet — Phase B).
- 12 NPCs: roles (2 witnesses, liar, keeper, hidden messenger, 7 walkers), jobs,
  home places, place ownership. Walker hints name first unfound trace's sector+side.
- Player: sector + x/y. Edge crossing wraps + logs entry + marks visited.

## 5. Key files

- `src/core/Game.cpp` — title/dialogue/win flow, E handling, sim-halt rules
- `src/simulation/Simulation.{h,cpp}` — gen, tick+travel, interact(), snapshot/restore
- `src/world/World.{h,cpp}` — districts + locations (sector-local coords)
- `src/core/Noise.h` — gradient noise + FBM + warp + isFlat (header-only)
- `src/rendering/Renderer.cpp` — one-sector view, biomes, minimap ring, boxes/heart
- `src/ui/Ui.cpp` + `UiOptions.h` — HUD, dialogue box, title/win screens
- `src/save/SaveSystem.{h,cpp}` — versioned save + migrations
- `src/entities/Npc.h` — occupation, homeLocation (static, not saved)
- `tests/test_determinism.cpp` — plain PASS/FAIL tests (no framework)
- Test helpers: `sim.travelTo(sector, pos)`, `sim.teleportPlayer(pos)`

## 6. Build / test / run (PowerShell, no `&&` — use `;`)

```
cmake --preset debug; cmake --build --preset debug
ctest --preset debug
.\build\debug\neta.exe --seed 482913
.\build\debug\neta.exe --headless --ticks 50 --seed 482913
```

## 7. Session routine (established pattern)

1. Build + `ctest` must pass before anything else.
2. Commit + push to `origin main` at end of session (clean source only;
   `build/`, exes, saves already gitignored).
3. Launch the game for the user: kill old process first (file lock!),
   then `Start-Process neta.exe --seed 482913`.
4. Reply simple: what changed, what to try, one question max.

## 8. Gotchas learned the hard way

- `Renderer::drawBox` FILLS black first — never call it for a frame around
  content you already drew; draw border-only rects instead.
- `npcLocalPos()` returns (-100,-100) for hidden messenger AND other-sector NPCs.
- Interact priority: traces → erase → NPCs (nearest) → places (smaller radius).
- Tests must `travelTo()` the right sector before teleport+interact (sector filter).
- Rebuilding while `neta.exe` runs fails (locked exe) — `Stop-Process -Name neta` first.
- LF/CRLF git warnings are harmless noise.
- Typewriter advances 2 chars/frame; sim halts while dialogue/title/win open.
- User runs the game THEMSELVES by playing the window we open; always confirm
  what they see with a screenshot request when visuals change.

## 9. Roadmap (in order discussed)

1. ~~Foundation~~ / ~~one-shot~~ / ~~rich city~~ / ~~ring biomes~~ / ~~living city~~ — DONE
2. **Phase B next:** solid generated structures (part-kit buildings, named giants,
   collision w/ slide, fair-placement nudges, `isFlat` gating). Visual pass first,
   then collision. Interiors NOT included.
3. KEEP-or-ERASE choice (two endings).
4. Danger + losing (hunters / countdown).
5. Detective depth (contradictions, notebook).
6. Sound + feel (letter blips, jingles, flashes).
7. Steam provider, achievements (far future).
