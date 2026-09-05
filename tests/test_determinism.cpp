// Foundation self-tests: no third-party framework, plain PASS/FAIL lines.
//   1. Same seed + same stream -> identical sequences (determinism).
//   2. Different seeds -> different sequences (seed actually matters).
//   3. Same seed + same tick count -> identical simulations.
//   4. Save -> load round-trip reproduces the snapshot, and the restored sim
//      continues identically to the original.
#include <cstdio>
#include <filesystem>
#include <string>

#include "core/Noise.h"
#include "core/Random.h"
#include "save/SaveSystem.h"
#include "simulation/Simulation.h"

namespace {

int g_failures = 0;

void check(bool ok, const char* name) {
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) {
        ++g_failures;
    }
}

bool streamsMatch(std::uint64_t seed, const char* streamName, int draws) {
    neta::core::Random a(seed);
    neta::core::Random b(seed);
    neta::core::RngStream sa = a.streamFor(streamName);
    neta::core::RngStream sb = b.streamFor(streamName);
    for (int i = 0; i < draws; ++i) {
        if (sa.nextU64() != sb.nextU64()) {
            return false;
        }
    }
    return true;
}

neta::save::SaveData runSim(std::uint64_t seed, int ticks) {
    neta::sim::Simulation sim;
    sim.generate(seed);
    for (int i = 0; i < ticks; ++i) {
        sim.tick(neta::sim::Simulation::kTickDt);
    }
    return sim.snapshot();
}

}  // namespace

int main() {
    check(streamsMatch(482913, "world", 64), "rng: same seed reproduces 'world' stream");
    check(streamsMatch(482913, "sim", 64), "rng: same seed reproduces 'sim' stream");
    check(streamsMatch(1, "world", 64), "rng: determinism holds for seed 1");

    {
        neta::core::Random a(482913);
        neta::core::Random b(482914);
        auto sa = a.streamFor("world");
        auto sb = b.streamFor("world");
        bool anyDifferent = false;
        for (int i = 0; i < 8; ++i) {
            if (sa.nextU64() != sb.nextU64()) {
                anyDifferent = true;
            }
        }
        check(anyDifferent, "rng: different seeds diverge");
    }

    {
        const auto s1 = runSim(482913, 50);
        const auto s2 = runSim(482913, 50);
        check(neta::save::snapshotsEqual(s1, s2), "sim: same seed + ticks -> identical snapshot");
    }

    {
        const auto s1 = runSim(482913, 50);
        const auto s3 = runSim(777, 50);
        check(!neta::save::snapshotsEqual(s1, s3), "sim: different seeds -> different worlds");
    }

    {
        // One-shot spots are deterministic per seed.
        neta::sim::Simulation a;
        neta::sim::Simulation b;
        a.generate(482913);
        b.generate(482913);
        bool same = a.traces().size() == 3 && b.traces().size() == 3;
        for (std::size_t i = 0; same && i < a.traces().size(); ++i) {
            same = same && a.traces()[i].district == b.traces()[i].district &&
                   a.traces()[i].pos.x == b.traces()[i].pos.x &&
                   a.traces()[i].pos.y == b.traces()[i].pos.y;
        }
        same = same && a.eraseTerminal().district == b.eraseTerminal().district;
        check(same, "oneshot: same seed -> same trace + erase spots");
        neta::sim::Simulation c;
        c.generate(777);
        bool diff = c.traces().size() == 3;
        bool anySpotDiff = false;
        for (std::size_t i = 0; diff && i < 3; ++i) {
            if (c.traces()[i].district != a.traces()[i].district) {
                anySpotDiff = true;
            }
        }
        check(diff && (anySpotDiff || c.eraseTerminal().district != a.eraseTerminal().district),
              "oneshot: different seeds -> different spots");
    }

    {
        // Full one-shot flow via teleports: erase locked -> 3 traces -> win.
        neta::sim::Simulation sim;
        sim.generate(482913);
        check(sim.tracesFound() == 0 && !sim.won(), "oneshot: starts with 0 traces, not won");
        check(sim.playerSector() == 0, "ring: player starts in sector 0");
        sim.travelTo(sim.eraseTerminal().district, sim.eraseTerminal().pos);
        auto locked = sim.interact();
        check(locked.kind == neta::sim::Simulation::InteractKind::EraseLocked,
              "oneshot: erase locked before 3 traces");
        check(!sim.won(), "oneshot: not won when locked");
        for (int t = 0; t < 3; ++t) {
            const auto& spot = sim.traces()[static_cast<std::size_t>(t)];
            sim.travelTo(spot.district, spot.pos);
            auto r = sim.interact();
            check(r.kind == neta::sim::Simulation::InteractKind::Trace,
                  "oneshot: trace pickup returns text");
            check(sim.tracesFound() == t + 1, "oneshot: trace count grows");
        }
        check(sim.messengerSpawned(), "oneshot: messenger spawns after 2 traces");
        check(static_cast<int>(sim.narrative().stage()) == 4,
              "oneshot: 3 traces -> ErasureKnown stage");
        sim.travelTo(sim.eraseTerminal().district, sim.eraseTerminal().pos);
        auto win = sim.interact();
        check(win.kind == neta::sim::Simulation::InteractKind::EraseWin,
              "oneshot: erase wins with 3 traces");
        check(sim.won(), "oneshot: won flag set");
    }

    {
        // v1-style save (empty one-shot vectors) restores as fresh progress.
        neta::sim::Simulation sim;
        sim.generate(482913);
        neta::save::SaveData v1 = sim.snapshot();
        v1.traces.clear();
        v1.talked.clear();
        v1.messengerSpawned = 0;
        v1.won = 0;
        neta::sim::Simulation into;
        into.generate(777);
        std::string err;
        check(into.restore(v1, err), "save: v1-style snapshot restores");
        check(into.tracesFound() == 0 && !into.won(), "save: v1 migrates to 0 traces, not won");
    }

    {
        // City: same seed -> same places; old district names untouched.
        neta::sim::Simulation a;
        neta::sim::Simulation b;
        a.generate(482913);
        b.generate(482913);
        bool same = a.world().locations().size() >= 15 &&
                    a.world().locations().size() == b.world().locations().size();
        for (std::size_t i = 0; same && i < a.world().locations().size(); ++i) {
            const auto& x = a.world().locations()[i];
            const auto& y = b.world().locations()[i];
            same = same && x.name == y.name && x.district == y.district && x.x == y.x &&
                   x.y == y.y;
        }
        check(same, "city: same seed -> same places");
        bool namesOk = a.world().districtCount() == 5 && !a.world().district(0).name.empty();
        check(namesOk, "city: 5 districts with names");
        neta::sim::Simulation c;
        c.generate(777);
        check(c.world().locations().size() != a.world().locations().size() ||
                  c.world().locations()[0].name != a.world().locations()[0].name,
              "city: different seeds -> different places");
    }

    {
        // Population: jobs + homes assigned, walkers hint at real traces.
        neta::sim::Simulation sim;
        sim.generate(482913);
        bool jobsOk = true;
        for (const auto& npc : sim.npcs()) {
            jobsOk = jobsOk && !npc.occupation().empty() && npc.homeLocation() >= 0;
        }
        check(jobsOk, "pop: every npc has job + home");
        // Walker hint mentions a real unfound trace district.
        bool hintOk = false;
        bool sectorWordOk = false;
        for (std::size_t i = 5; i < sim.npcs().size() && !hintOk; ++i) {
            sim.travelTo(sim.npcs()[i].district(), {50.0f, 28.0f});
            sim.teleportPlayer(sim.npcLocalPos(i));
            auto r = sim.interact();
            if (r.kind != neta::sim::Simulation::InteractKind::Talk) {
                continue;
            }
            for (const auto& t : sim.traces()) {
                const std::string& dname = sim.world().district(t.district).name;
                if (r.text.find(dname) != std::string::npos) {
                    hintOk = true;
                    break;
                }
            }
            if (r.text.find("SECTOR") != std::string::npos) {
                sectorWordOk = true;
            }
        }
        check(hintOk, "pop: walker hint names a trace district");
        check(sectorWordOk, "pop: walker hint uses SECTOR coordinates");
        // Examining a far-flung place shows its description.
        bool placeOk = false;
        {
            // Find the place furthest from all higher-priority interactables.
            auto dist = [](float ax, float ay, float bx, float by) {
                const float dx = ax - bx;
                const float dy = ay - by;
                return dx * dx + dy * dy;
            };
            const auto& locs = sim.world().locations();
            std::size_t pick = 0;
            float pickD2 = -1.0f;
            for (const auto& loc : locs) {
                // Stand in the place's sector so npc positions resolve.
                sim.travelTo(loc.district, {loc.x, loc.y});
                float nearest = 1e20f;
                if (sim.eraseTerminal().district == loc.district) {
                    nearest = dist(loc.x, loc.y, sim.eraseTerminal().pos.x,
                                   sim.eraseTerminal().pos.y);
                }
                for (const auto& t : sim.traces()) {
                    if (t.district != loc.district) {
                        continue;
                    }
                    const float d2 = dist(loc.x, loc.y, t.pos.x, t.pos.y);
                    if (d2 < nearest) {
                        nearest = d2;
                    }
                }
                for (std::size_t i = 0; i < sim.npcs().size(); ++i) {
                    const auto wp = sim.npcLocalPos(i);
                    if (wp.x < -50.0f) {
                        continue;
                    }
                    const float d2 = dist(loc.x, loc.y, wp.x, wp.y);
                    if (d2 < nearest) {
                        nearest = d2;
                    }
                }
                if (nearest > pickD2) {
                    pickD2 = nearest;
                    pick = static_cast<std::size_t>(loc.id);
                }
            }
            if (pickD2 > 7.5f * 7.5f) {
                const auto& loc = locs[pick];
                neta::sim::Simulation probe;
                probe.generate(482913);
                probe.travelTo(loc.district, {loc.x, loc.y});
                auto r = probe.interact();
                placeOk = (r.kind == neta::sim::Simulation::InteractKind::Place && !r.text.empty());
            }
        }
        check(placeOk, "city: examining a place shows text");
    }

    {
        // Ring travel: walking off an edge enters the next sector, wraps.
        neta::sim::Simulation sim;
        sim.generate(482913);
        sim.teleportPlayer({99.0f, 28.0f});
        sim.setPlayerIntent({1.0f, 0.0f});
        sim.tick(neta::sim::Simulation::kTickDt);
        check(sim.playerSector() == 1, "ring: east edge enters next sector");
        check(sim.visited(1) == 1, "ring: entered sector marked visited");
        sim.travelTo(0, {1.0f, 28.0f});
        sim.setPlayerIntent({-1.0f, 0.0f});
        sim.tick(neta::sim::Simulation::kTickDt);
        check(sim.playerSector() == 4, "ring: west edge from 0 wraps to 4");
        sim.travelTo(4, {99.0f, 28.0f});
        sim.setPlayerIntent({1.0f, 0.0f});
        sim.tick(neta::sim::Simulation::kTickDt);
        check(sim.playerSector() == 0, "ring: east edge from 4 wraps to 0");
    }

    {
        // Noise: deterministic, bounded, sane math, useful slope answers.
        const neta::noise::Preset p{4, 0.5f, 2.0f, 0.6f, 16.0f};
        check(neta::noise::noise2(482913, 3.25f, 7.75f) ==
                  neta::noise::noise2(482913, 3.25f, 7.75f),
              "noise: same inputs -> same value");
        check(neta::noise::warped(482913, 3.25f, 7.75f, p) !=
                  neta::noise::warped(777, 3.25f, 7.75f, p),
              "noise: different seeds diverge");
        bool bounded = true;
        for (int i = 0; i < 64; ++i) {
            const float v = neta::noise::warped(
                482913, static_cast<float>(i) * 1.7f, static_cast<float>(i) * 0.6f, p);
            if (v < -1.0f || v > 1.0f) {
                bounded = false;
            }
        }
        check(bounded, "noise: warped stays in [-1, 1]");
        const neta::noise::Preset one{1, 0.5f, 2.0f, 0.0f, 16.0f};
        check(neta::noise::fbm(99, 5.5f, 2.5f, one) == neta::noise::noise2(99, 5.5f / 16.0f, 2.5f / 16.0f),
              "noise: 1-octave fbm equals base noise");
        check(neta::noise::isFlat(7, 1.0f, 1.0f, p, 1.0f, 100.0f),
              "noise: huge limit is always flat");
        check(!neta::noise::isFlat(7, 1.0f, 1.0f, p, 1.0f, 0.0f),
              "noise: zero limit is never flat");
    }

    {
        // Turf + owners: valid, deterministic, same-district owners.
        neta::sim::Simulation a;
        neta::sim::Simulation b;
        a.generate(482913);
        b.generate(482913);
        bool turfOk = true;
        for (int s = 0; s < 5; ++s) {
            turfOk = turfOk && a.turf(s) == b.turf(s) && a.turf(s) >= 0 && a.turf(s) < 4;
        }
        check(turfOk, "turf: same seed -> same valid factions");
        bool ownersOk = a.world().locations().size() == b.world().locations().size();
        for (std::size_t i = 0; ownersOk && i < a.world().locations().size(); ++i) {
            const int o = a.placeOwner(static_cast<int>(i));
            ownersOk = ownersOk && o >= 0 && o == b.placeOwner(static_cast<int>(i)) &&
                       a.npcs()[static_cast<std::size_t>(o)].district() ==
                           a.world().locations()[i].district;
        }
        check(ownersOk, "city: owners valid, deterministic, same district");
        // Walkers name the place they keep.
        bool mindOk = false;
        for (std::size_t i = 5; i < a.npcs().size() && !mindOk; ++i) {
            a.travelTo(a.npcs()[i].district(), {50.0f, 28.0f});
            a.teleportPlayer(a.npcLocalPos(i));
            auto r = a.interact();
            if (r.kind == neta::sim::Simulation::InteractKind::Talk &&
                r.text.find("MIND") != std::string::npos) {
                mindOk = true;
            }
        }
        check(mindOk, "pop: walkers name the place they keep");
    }

    {
        // Save v3 remembers the sector; v2-style restores to sector 0.
        neta::sim::Simulation sim;
        sim.generate(482913);
        sim.travelTo(3, {50.0f, 28.0f});
        const auto snap = sim.snapshot();
        check(snap.playerSector == 3, "save: snapshot carries sector");
        neta::sim::Simulation into;
        into.generate(482913);
        std::string err;
        check(into.restore(snap, err), "save: v3 snapshot restores");
        check(into.playerSector() == 3, "save: restore keeps sector");
        neta::save::SaveData v2 = snap;
        v2.playerSector = 0;
        v2.visited.clear();
        neta::sim::Simulation into2;
        into2.generate(482913);
        check(into2.restore(v2, err), "save: v2-style snapshot restores");
        check(into2.playerSector() == 0, "save: v2 migrates to sector 0");
    }

    {
        // Round-trip through the real file format in the OS temp dir.
        const auto before = runSim(482913, 42);
        const std::string path =
            (std::filesystem::temp_directory_path() / "neta_test_roundtrip.nsave").string();
        bool ok = neta::save::SaveSystem::saveToFile(path, before);
        neta::save::SaveData loaded;
        std::string error;
        ok = ok && neta::save::SaveSystem::loadFromFile(path, loaded, error);
        check(ok, "save: file round-trip loads");
        if (!ok) {
            std::printf("  error: %s\n", error.c_str());
        }
        check(neta::save::snapshotsEqual(before, loaded), "save: snapshot survives round-trip");

        // Continuation check: restored sim + original, ticked equally, agree.
        neta::sim::Simulation cont;
        cont.generate(482913);
        std::string rerr;
        check(cont.restore(loaded, rerr), "save: restore into simulation");
        neta::sim::Simulation orig;
        orig.generate(482913);
        for (int i = 0; i < 42; ++i) {
            orig.tick(neta::sim::Simulation::kTickDt);
        }
        for (int i = 0; i < 20; ++i) {
            orig.tick(neta::sim::Simulation::kTickDt);
            cont.tick(neta::sim::Simulation::kTickDt);
        }
        check(neta::save::snapshotsEqual(orig.snapshot(), cont.snapshot()),
              "save: restored sim continues identically");
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    if (g_failures == 0) {
        std::printf("ALL TESTS PASSED\n");
    } else {
        std::printf("%d TEST(S) FAILED\n", g_failures);
    }
    return g_failures == 0 ? 0 : 1;
}
