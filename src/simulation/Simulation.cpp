#include "simulation/Simulation.h"

#include <format>

#include "core/Log.h"

namespace neta::sim {

namespace {
constexpr const char* kFactionNames[] = {"HELIX CARTEL", "CIVIC GRID", "RUST CHAPEL", "MERIDIAN GROUP"};
constexpr const char* kFactionAgendas[] = {
    "control dockside trade", "keep the grid online", "spread the signal", "buy every secret"};
constexpr const char* kNpcNames[] = {"K.VOSS", "MARA-J", "IBN-S", "OKAFOR", "VEX-9", "SABLE",
                                     "HUSH", "CORVIN", "DARA-O", "NULL-K", "PETRA-V", "JUNO-3"};
constexpr std::size_t kMaxEvents = 64;
}  // namespace

Simulation::Simulation()
    : m_rng(core::Random{0}.streamFor("sim")) {}

void Simulation::generate(std::uint64_t seed) {
    m_seed = seed;
    m_tick = 0;
    m_time = 0.0;
    m_events.clear();
    m_intent = {};
    m_narrative.reset();
    buildStaticContent(seed);
    pushEvent(std::format("WORLD GENERATED seed={}", seed));
    core::Log::info("sim", "world generated: seed={} districts={} factions={} npcs={}", seed,
                    m_world.districtCount(), m_factions.size(), m_npcs.size());
}

void Simulation::buildStaticContent(std::uint64_t seed) {
    m_world.generate(seed);
    core::Random rng(seed);
    core::RngStream gen = rng.streamFor("sim");
    m_rng = rng.streamFor("sim");  // live stream restarts identically

    m_factions.clear();
    for (std::size_t i = 0; i < kFactionCount; ++i) {
        m_factions.emplace_back(static_cast<int>(i), kFactionNames[i], kFactionAgendas[i]);
    }
    // One illustrative rivalry so the attitude table has non-zero content.
    m_factions[0].setRelation(2, -40);
    m_factions[2].setRelation(0, -40);

    m_npcs.clear();
    for (std::uint32_t i = 0; i < kNpcCount; ++i) {
        entities::Npc npc(i + 1, kNpcNames[i % 12]);
        npc.setFactionId(static_cast<int>(gen.rangeIndex(kFactionCount)));
        npc.setDistrict(static_cast<int>(gen.rangeIndex(world::World::kDistrictCount)));
        npc.setLocal(static_cast<float>(gen.nextDouble()), static_cast<float>(gen.nextDouble()));
        m_npcs.push_back(std::move(npc));
    }
    m_player = entities::Player{};

    // One-shot setup on its own stream so world/sim names never reshuffle.
    m_roles.assign(kNpcCount, oneshot::NpcRole::Walker);
    m_talked.assign(kNpcCount, 0);
    m_traces.clear();
    m_messengerSpawned = false;
    m_won = false;
    if (!m_npcs.empty()) {
        using R = oneshot::NpcRole;
        m_roles[0] = R::WitnessA;
        m_roles[1] = R::WitnessB;
        m_roles[2] = R::Liar;
        m_roles[3] = R::Keeper;
        m_roles[4] = R::Messenger;
    }
    core::RngStream os = rng.streamFor("oneshot");
    const int nDistricts = m_world.districtCount() > 0 ? m_world.districtCount() : 1;
    // Pick 3 distinct trace districts, then an erase district from the rest.
    std::vector<int> order;
    for (int i = 0; i < nDistricts; ++i) {
        order.push_back(i);
    }
    for (int i = static_cast<int>(order.size()) - 1; i > 0; --i) {
        const int j = os.rangeInt(0, i);
        const int tmp = order[static_cast<std::size_t>(i)];
        order[static_cast<std::size_t>(i)] = order[static_cast<std::size_t>(j)];
        order[static_cast<std::size_t>(j)] = tmp;
    }
    const float colW = kAreaW / static_cast<float>(nDistricts);
    for (int t = 0; t < oneshot::kTraceCount && t < static_cast<int>(order.size()); ++t) {
        TraceSpot spot;
        spot.id = t;
        spot.district = order[static_cast<std::size_t>(t)];
        const double fx = 0.3 + os.nextDouble() * 0.4;
        const double fy = 0.25 + os.nextDouble() * 0.5;
        spot.pos.x = (static_cast<float>(spot.district) + static_cast<float>(fx)) * colW;
        spot.pos.y = 6.0f + static_cast<float>(fy) * (kAreaH - 12.0f);
        spot.found = false;
        m_traces.push_back(spot);
    }
    int eraseDistrict = nDistricts - 1;
    if (static_cast<int>(order.size()) > oneshot::kTraceCount) {
        eraseDistrict =
            order[static_cast<std::size_t>(oneshot::kTraceCount + os.rangeIndex(order.size() -
                                                                               oneshot::kTraceCount))];
    }
    m_erase.district = eraseDistrict;
    m_erase.pos.x = (static_cast<float>(eraseDistrict) + 0.5f) * colW;
    m_erase.pos.y = kAreaH * 0.5f;

    // Place special NPCs near their story spots so they can be found.
    auto placeNear = [&](std::size_t idx, entities::Vec2 target, float dx, float dy) {
        if (idx >= m_npcs.size()) {
            return;
        }
        float x = target.x + dx;
        float y = target.y + dy;
        if (x < 2.0f) {
            x = 2.0f;
        }
        if (x > kAreaW - 2.0f) {
            x = kAreaW - 2.0f;
        }
        if (y < 2.0f) {
            y = 2.0f;
        }
        if (y > kAreaH - 2.0f) {
            y = kAreaH - 2.0f;
        }
        // Convert world pos back to district + local for storage.
        int d = static_cast<int>(x / colW);
        if (d < 0) {
            d = 0;
        }
        if (d >= nDistricts) {
            d = nDistricts - 1;
        }
        const float lx = (x - static_cast<float>(d) * colW) / colW;
        const float ly = (y - 5.0f) / (kAreaH - 10.0f);
        m_npcs[idx].setDistrict(d);
        m_npcs[idx].setLocal(lx, ly);
    };
    if (m_traces.size() >= 3) {
        placeNear(0, m_traces[0].pos, 4.0f, 2.0f);
        placeNear(1, m_traces[1].pos, -4.0f, -2.0f);
        placeNear(2, m_traces[2].pos, 4.0f, -2.0f);
    }
    placeNear(3, m_erase.pos, -5.0f, 3.0f);
    placeNear(4, m_erase.pos, 5.0f, -3.0f);

    // Population depth on its own stream: jobs + a home place in the NPC's
    // own district. Static from the seed, so saves don't store it.
    static constexpr const char* kJobs[] = {"COURIER", "MEDIC",   "FIXER",  "DOCKER",
                                            "SPLICER", "VENDOR",  "DRIVER", "ARCHIVIST",
                                            "COOK",    "WATCHER", "MUSICIAN"};
    static constexpr std::size_t kJobCount = 11;
    core::RngStream pop = rng.streamFor("pop");
    for (std::size_t i = 0; i < m_npcs.size(); ++i) {
        m_npcs[i].setOccupation(kJobs[pop.rangeIndex(kJobCount)]);
        const int home = m_npcs[i].district();
        int first = -1;
        int count = 0;
        for (const auto& loc : m_world.locations()) {
            if (loc.district == home) {
                if (first < 0) {
                    first = loc.id;
                }
                ++count;
            }
        }
        if (count > 0) {
            m_npcs[i].setHomeLocation(first + static_cast<int>(pop.rangeIndex(
                                                          static_cast<std::size_t>(count))));
        }
    }
}

void Simulation::tick(double dt) {
    if (dt <= 0.0) {
        return;
    }
    ++m_tick;
    m_time += dt;

    if (!m_won) {
        m_player.move(m_intent, static_cast<float>(dt));
        clampPlayer();
    }

    const float fdt = static_cast<float>(dt);
    for (auto& npc : m_npcs) {
        npc.wander(m_rng, fdt);
    }

    // Stages now move only by player action in interact(), never by tick.
    // Quiet after winning: no more ambient plot noise.
    if (!m_won && m_tick % 25 == 0) {
        emitPlaceholderEvent();
    }
}

void Simulation::emitPlaceholderEvent() {
    // Illustrative event shapes in the exact style the future debug feed
    // will use. The systems behind them (secrets, diplomacy, event engine)
    // do not exist yet; the feed plumbing does.
    const int kind = m_rng.rangeInt(0, 4);
    const auto npcId = static_cast<std::uint32_t>(m_rng.rangeIndex(m_npcs.size()) + 1);
    switch (kind) {
        case 0: {
            const int f = m_rng.rangeInt(0, static_cast<int>(m_factions.size()) - 1);
            pushEvent(std::format("NPC {} joined FACTION {}", npcId, f));
            m_npcs[npcId - 1].setFactionId(f);
            break;
        }
        case 1: {
            const int s = m_rng.rangeInt(1, 9);
            pushEvent(std::format("NPC {} discovered SECRET {}", npcId, s));
            break;
        }
        case 2: {
            int a = m_rng.rangeInt(0, static_cast<int>(m_factions.size()) - 1);
            int b = m_rng.rangeInt(0, static_cast<int>(m_factions.size()) - 2);
            if (b >= a) {
                ++b;
            }
            pushEvent(std::format("FACTION {} became hostile toward FACTION {}", a, b));
            break;
        }
        case 3: {
            const int d = m_rng.rangeInt(0, m_world.districtCount() - 1);
            pushEvent(std::format("NPC {} changed location", npcId));
            m_npcs[npcId - 1].setDistrict(d);
            break;
        }
        default: {
            pushEvent(std::format("EVENT {} started", 1000 + m_tick));
            break;
        }
    }
    core::Log::debugText("sim", m_events.back());
}

void Simulation::pushEvent(const std::string& event) {
    m_events.push_back(event);
    while (m_events.size() > kMaxEvents) {
        m_events.pop_front();
    }
}

int Simulation::tracesFound() const {
    int n = 0;
    for (const auto& t : m_traces) {
        if (t.found) {
            ++n;
        }
    }
    return n;
}

entities::Vec2 Simulation::npcWorldPos(std::size_t i) const {
    if (i >= m_npcs.size()) {
        return {-100.0f, -100.0f};
    }
    if (m_roles.size() > i && m_roles[i] == oneshot::NpcRole::Messenger && !m_messengerSpawned) {
        return {-100.0f, -100.0f};  // hidden until 2 traces
    }
    const auto& npc = m_npcs[i];
    const int n = m_world.districtCount() > 0 ? m_world.districtCount() : 1;
    const float colW = kAreaW / static_cast<float>(n);
    int d = npc.district();
    if (d < 0) {
        d = 0;
    }
    if (d >= n) {
        d = n - 1;
    }
    entities::Vec2 p;
    p.x = (static_cast<float>(d) + 0.1f + npc.localX() * 0.8f) * colW;
    p.y = 5.0f + npc.localY() * (kAreaH - 10.0f);
    return p;
}

Simulation::InteractResult Simulation::interact() {
    InteractResult out;
    const entities::Vec2 pp = m_player.position();
    auto dist2 = [](entities::Vec2 a, entities::Vec2 b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    };
    const float r2 = kInteractRadius * kInteractRadius;

    // 1) Traces first.
    for (auto& t : m_traces) {
        if (!t.found && dist2(pp, t.pos) <= r2) {
            t.found = true;
            const int found = tracesFound();
            if (found == 1) {
                m_narrative.setStage(narrative::ObjectiveStage::Aware);
                pushEvent("PLAYER found TRACE 1/3");
            } else if (found == 2) {
                m_narrative.setStage(narrative::ObjectiveStage::Investigating);
                m_messengerSpawned = true;
                pushEvent("PLAYER found TRACE 2/3, MESSENGER appeared");
            } else if (found >= 3) {
                m_narrative.setStage(narrative::ObjectiveStage::ErasureKnown);
                pushEvent("PLAYER found TRACE 3/3, ERASE unlocked");
            }
            out.kind = InteractKind::Trace;
            out.index = t.id;
            out.speaker = oneshot::kTraces[static_cast<std::size_t>(t.id)].title;
            out.text = oneshot::kTraces[static_cast<std::size_t>(t.id)].text;
            return out;
        }
    }

    // 2) Erase terminal.
    if (dist2(pp, m_erase.pos) <= r2) {
        if (tracesFound() >= oneshot::kTraceCount) {
            if (!m_won) {
                m_won = true;
                m_narrative.setStage(narrative::ObjectiveStage::ErasureKnown);
                pushEvent("PLAYER ERASED THE APPLICATION");
            }
            out.kind = InteractKind::EraseWin;
            out.speaker = "ERASE TERMINAL";
            out.text = oneshot::eraseWinText();
            return out;
        }
        out.kind = InteractKind::EraseLocked;
        out.speaker = "ERASE TERMINAL";
        out.text = oneshot::eraseLockedText();
        return out;
    }

    // 3) NPCs, nearest first.
    int best = -1;
    float bestD2 = r2;
    for (std::size_t i = 0; i < m_npcs.size(); ++i) {
        if (m_roles.size() > i && m_roles[i] == oneshot::NpcRole::Messenger &&
            !m_messengerSpawned) {
            continue;
        }
        const float d2 = dist2(pp, npcWorldPos(i));
        if (d2 <= bestD2) {
            bestD2 = d2;
            best = static_cast<int>(i);
        }
    }
    if (best >= 0) {
        const std::size_t bi = static_cast<std::size_t>(best);
        m_talked[bi] = 1;
        const oneshot::NpcRole role =
            bi < m_roles.size() ? m_roles[bi] : oneshot::NpcRole::Walker;
        out.index = best;
        out.speaker = std::string(m_npcs[bi].name()) + ", " + m_npcs[bi].occupation() + " (" +
                      oneshot::roleName(role) + ")";
        switch (role) {
            case oneshot::NpcRole::WitnessA:
                out.kind = InteractKind::Talk;
                out.text = oneshot::witnessAText();
                pushEvent(std::format("PLAYER talked to WITNESS {}", m_npcs[bi].name()));
                break;
            case oneshot::NpcRole::WitnessB:
                out.kind = InteractKind::Talk;
                out.text = oneshot::witnessBText();
                pushEvent(std::format("PLAYER talked to WITNESS {}", m_npcs[bi].name()));
                break;
            case oneshot::NpcRole::Liar:
                out.kind = InteractKind::Talk;
                out.text = oneshot::liarText();
                pushEvent(std::format("PLAYER talked to BELIEVER {}", m_npcs[bi].name()));
                break;
            case oneshot::NpcRole::Keeper:
                out.kind = InteractKind::Talk;
                out.text = oneshot::keeperText();
                pushEvent(std::format("PLAYER talked to KEEPER {}", m_npcs[bi].name()));
                break;
            case oneshot::NpcRole::Messenger:
                out.kind = InteractKind::Messenger;
                out.text = oneshot::messengerText();
                if (m_narrative.stage() != narrative::ObjectiveStage::ErasureKnown) {
                    m_narrative.setStage(narrative::ObjectiveStage::Contact3155);
                }
                pushEvent("PLAYER met the 3155 MESSENGER");
                break;
            case oneshot::NpcRole::Walker:
            default: {
                out.kind = InteractKind::Talk;
                // True hint: name the district of the first still-hidden trace
                // so explorers can navigate by talk. The liar still misleads.
                const world::District* target = nullptr;
                for (const auto& t : m_traces) {
                    if (!t.found) {
                        target = &m_world.district(t.district);
                        break;
                    }
                }
                if (target == nullptr) {
                    out.text = oneshot::walkerLine();
                } else if (best % 3 == 0) {
                    out.text = std::format(
                        "* I AM {} AROUND HERE. FOLKS SAY SOMETHING STRANGE HIDES PAST {}.",
                        m_npcs[bi].occupation(), target->name);
                } else if (best % 3 == 1) {
                    out.text = std::format(
                        "* YOU LOOKING FOR OLD RECORDS? TRY {}. I HEAR STATIC FROM THAT WAY.",
                        target->name);
                } else {
                    out.text = std::format(
                        "* {} GIVES ME BAD DREAMS. TOO QUIET. GO LOOK THERE YOURSELF.",
                        target->name);
                }
                break;
            }
        }
        return out;
    }

    // 4) Named places: examine for a short description. Smaller radius so
    // people (checked above) always win ties.
    {
        const float pr = 5.0f;
        const float pr2 = pr * pr;
        int bestPlace = -1;
        float bestPlaceD2 = pr2;
        for (const auto& loc : m_world.locations()) {
            const float d2 = dist2(pp, {loc.x, loc.y});
            if (d2 <= bestPlaceD2) {
                bestPlaceD2 = d2;
                bestPlace = loc.id;
            }
        }
        if (bestPlace >= 0) {
            const auto& loc =
                m_world.location(bestPlace);
            out.kind = InteractKind::Place;
            out.index = bestPlace;
            out.speaker = loc.name + " (" + loc.kind + ")";
            out.text = loc.desc;
            return out;
        }
    }

    return out;
}

void Simulation::clampPlayer() {
    entities::Vec2 p = m_player.position();
    if (p.x < 0.0f) {
        p.x = 0.0f;
    } else if (p.x > kAreaW) {
        p.x = kAreaW;
    }
    if (p.y < 0.0f) {
        p.y = 0.0f;
    } else if (p.y > kAreaH) {
        p.y = kAreaH;
    }
    m_player.setPosition(p);
}

save::SaveData Simulation::snapshot() const {
    save::SaveData data;
    data.seed = m_seed;
    data.tick = m_tick;
    data.rngState = m_rng.state();
    data.time = m_time;
    data.playerX = m_player.position().x;
    data.playerY = m_player.position().y;
    data.stage = static_cast<int>(m_narrative.stage());
    data.npcs.reserve(m_npcs.size());
    for (const auto& npc : m_npcs) {
        save::NpcSave n;
        n.id = npc.id();
        n.faction = npc.factionId();
        n.district = npc.district();
        n.lx = npc.localX();
        n.ly = npc.localY();
        n.action = npc.currentAction();
        data.npcs.push_back(std::move(n));
    }
    data.traces.assign(oneshot::kTraceCount, 0);
    for (const auto& t : m_traces) {
        if (t.id >= 0 && t.id < oneshot::kTraceCount) {
            data.traces[static_cast<std::size_t>(t.id)] = t.found ? 1 : 0;
        }
    }
    data.talked = m_talked;
    data.messengerSpawned = m_messengerSpawned ? 1 : 0;
    data.won = m_won ? 1 : 0;
    return data;
}

bool Simulation::restore(const save::SaveData& data, std::string& error) {
    if (data.npcs.size() != kNpcCount) {
        error = "save has " + std::to_string(data.npcs.size()) + " npcs, expected " +
                std::to_string(kNpcCount);
        return false;
    }
    if (data.stage < 0 || data.stage > 4) {
        error = "save has invalid objective stage";
        return false;
    }
    // Rebuild static content from the seed, then overwrite dynamic fields.
    const std::uint64_t seed = data.seed;
    buildStaticContent(seed);
    m_seed = seed;
    m_tick = data.tick;
    m_time = data.time;
    m_rng.setState(data.rngState);
    m_events.clear();
    m_intent = {};
    m_player.setPosition({data.playerX, data.playerY});
    clampPlayer();
    m_narrative.setStage(static_cast<narrative::ObjectiveStage>(data.stage));
    for (std::size_t i = 0; i < m_npcs.size(); ++i) {
        m_npcs[i].setFactionId(data.npcs[i].faction);
        m_npcs[i].setDistrict(data.npcs[i].district);
        m_npcs[i].setLocal(data.npcs[i].lx, data.npcs[i].ly);
        m_npcs[i].setCurrentAction(data.npcs[i].action);
    }
    // One-shot flags: v1 saves carry empty vectors (migrated to zeros).
    if (data.traces.size() == static_cast<std::size_t>(oneshot::kTraceCount)) {
        for (std::size_t i = 0; i < m_traces.size(); ++i) {
            const int v = data.traces[i];
            m_traces[i].found = (v != 0);
        }
    }
    if (data.talked.size() == m_talked.size()) {
        m_talked = data.talked;
    }
    m_messengerSpawned = (data.messengerSpawned != 0);
    m_won = (data.won != 0);
    pushEvent(std::format("SAVE LOADED tick={}", m_tick));
    return true;
}

}  // namespace neta::sim
