// Foundation self-tests: no third-party framework, plain PASS/FAIL lines.
//   1. Same seed + same stream -> identical sequences (determinism).
//   2. Different seeds -> different sequences (seed actually matters).
//   3. Same seed + same tick count -> identical simulations.
//   4. Save -> load round-trip reproduces the snapshot, and the restored sim
//      continues identically to the original.
#include <cstdio>
#include <filesystem>
#include <string>

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
        sim.teleportPlayer(sim.eraseTerminal().pos);
        auto locked = sim.interact();
        check(locked.kind == neta::sim::Simulation::InteractKind::EraseLocked,
              "oneshot: erase locked before 3 traces");
        check(!sim.won(), "oneshot: not won when locked");
        for (int t = 0; t < 3; ++t) {
            sim.teleportPlayer(sim.traces()[static_cast<std::size_t>(t)].pos);
            auto r = sim.interact();
            check(r.kind == neta::sim::Simulation::InteractKind::Trace,
                  "oneshot: trace pickup returns text");
            check(sim.tracesFound() == t + 1, "oneshot: trace count grows");
        }
        check(sim.messengerSpawned(), "oneshot: messenger spawns after 2 traces");
        check(static_cast<int>(sim.narrative().stage()) == 4,
              "oneshot: 3 traces -> ErasureKnown stage");
        sim.teleportPlayer(sim.eraseTerminal().pos);
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
        for (std::size_t i = 5; i < sim.npcs().size() && !hintOk; ++i) {
            sim.teleportPlayer(sim.npcWorldPos(i));
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
        }
        check(hintOk, "pop: walker hint names a trace district");
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
                float nearest = dist(loc.x, loc.y, sim.eraseTerminal().pos.x,
                                     sim.eraseTerminal().pos.y);
                for (const auto& t : sim.traces()) {
                    const float d2 = dist(loc.x, loc.y, t.pos.x, t.pos.y);
                    if (d2 < nearest) {
                        nearest = d2;
                    }
                }
                for (std::size_t i = 0; i < sim.npcs().size(); ++i) {
                    const auto wp = sim.npcWorldPos(i);
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
                probe.teleportPlayer({loc.x, loc.y});
                auto r = probe.interact();
                placeOk = (r.kind == neta::sim::Simulation::InteractKind::Place && !r.text.empty());
            }
        }
        check(placeOk, "city: examining a place shows text");
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
