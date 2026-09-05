# NETA-2090 — Technical Foundation (v0.1.0)

Retro cyberpunk narrative/simulation game for Windows PC (eventual Steam release).
This repository currently holds the **technical foundation only**: window, game loop,
deterministic simulation core, placeholder world/entities, save system, and debug tooling.
The full procedural narrative simulation is designed but intentionally not implemented yet.

> Year 2090. Humanity believes the rogue-AI crisis of the 2020s is ancient history.
> You find an application that should not exist — one that reaches across past,
> present, and future. People from 3155 say it must be erased.
> Objective: **ERASE THE APPLICATION.** How is up to the world — and you.

## Prerequisites (Windows)

| Tool | Install |
|---|---|
| GCC (MinGW UCRT) | `winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT` |
| CMake ≥ 3.20 | `winget install -e --id Kitware.CMake` |
| Ninja | `winget install -e --id Ninja-build.Ninja` |
| VS Code (optional) | `winget install -e --id Microsoft.VisualStudioCode` |
| GDB (optional, ships with WinLibs) | on `PATH` after the WinLibs install |

SDL2 is **downloaded automatically** at configure time (official libsdl-org MinGW
binaries, SHA-256 pinned in `CMakeLists.txt`) — nothing to preinstall.
After installing tools, restart your terminal so `PATH` updates apply.

## Build

```powershell
cmake --preset debug        # first time: downloads SDL2 (~10 MB)
cmake --build --preset debug
```

Release build (distributable, no debug overhead):

```powershell
cmake --preset release
cmake --build --preset release
```

Binaries land in `build/debug/` and `build/release/`. `SDL2.dll` is copied next to
`neta.exe` automatically, so a Release folder runs on a clean Windows machine.

## Run

```powershell
.\build\debug\neta.exe                      # windowed game
.\build\debug\neta.exe --seed 482913        # same seed = same generated world
.\build\debug\neta.exe --headless --ticks 60 --seed 482913   # sim only, no window
.\build\debug\neta.exe --help               # all flags
```

### Controls

| Key | Action |
|---|---|
| WASD / arrows | Move |
| E / Z / Enter | Talk, pick up trace, advance text |
| Space | Pause / resume simulation (when no text box open) |
| F1 | Toggle debug event overlay |
| F2 | Toggle CRT scanlines |
| F5 | Quicksave |
| F9 | Quickload |
| Esc / window X | Quit |

### Saves

Quicksaves go to the OS per-user dir (e.g.
`%APPDATA%\neta\neta-foundation\saves\quicksave.nsave`), falling back to `./saves/`.
The format is versioned (`NETASAVE 1`); see `DEVELOPMENT.md` for the migration policy.

## Tests

```powershell
ctest --preset debug        # determinism tests + headless sim smoke test
```

This verifies: same seed → identical RNG streams and worlds, different seeds diverge,
and save → load reproduces the exact snapshot (including continued simulation).

## Project layout

```
src/
  main/        entry point
  core/        Game loop, Clock, Config, Log, Random (seeded RNG)
  simulation/  headless world simulation (owns truth)
  world/       districts / city placeholder
  entities/    Entity, Player, Npc, Faction
  ai/          AgentMind STUB (future goals/planning/beliefs)
  narrative/   Canon (fixed facts) + NarrativeState (objective tracker)
  rendering/   SDL2 renderer, 480x270 retro framebuffer, built-in pixel font
  input/       keyboard/mouse pump
  audio/       STUB (lifecycle slots only)
  ui/          HUD + debug overlay
  save/        versioned save/load
  platform/    Steam seam (NullSteamProvider; offline by default)
tests/         determinism + save round-trip self-tests
```

See **`DEVELOPMENT.md`** for architecture, contracts, and the roadmap.
