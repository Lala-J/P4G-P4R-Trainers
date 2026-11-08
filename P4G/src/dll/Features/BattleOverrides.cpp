#include "Features/BattleOverrides.h"

#include <cstdint>

#include "Engine/Addresses.h"
#include "Engine/GameTypes.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"
#include "Features/Battle.h"
#include "Hooks/Hook.h"
#include "Util/Log.h"
#include "Util/Settings.h"

namespace BattleFx {

namespace {

Options g_o;

constexpr uint32_t kWeak = 0x08000000, kResist = 0x10000000, kNull = 0x01000000;

using Resolve_t = int (*)(Game::Unit*, Game::Unit*, uint16_t, uint32_t);
using Aff_t = uint32_t (*)(Game::Unit*, short);
using Rand_t = uint32_t (*)();
using Ai_t = int (*)();
using Can_t = int (*)(void*, short);
using Known_t = uint64_t (*)(uint16_t, uint16_t);

Resolve_t o_Hit = nullptr, o_Crit = nullptr;
Aff_t o_Aff = nullptr;
Rand_t o_Rand = nullptr;
Ai_t o_Flee = nullptr;
Ai_t g_wait = nullptr;  // AI_WAIT's own code
Can_t o_Can = nullptr;
Known_t o_Known = nullptr;

thread_local int t_rollZero = 0;  // >0 while a party attack's crit roll should land

bool Party(const Game::Unit* u) { return u && Battle::IsPartyUnit(u); }

int Hit_Hook(Game::Unit* atk, Game::Unit* tgt, uint16_t skill, uint32_t flags) {
    Hooks::Guard g;
    int r = o_Hit(atk, tgt, skill, flags);
    bool atkParty = Party(atk), tgtParty = Party(tgt);
    if (g_o.enemiesMiss && tgtParty && !atkParty && !(flags & 2) && r == 1) {
        *reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(tgt) + 0x30) = 0;  // plain miss text
        return 4;
    }
    if (g_o.partyHits && atkParty && !tgtParty && (r == 4 || r == 0x200)) return 1;
    return r;
}

int Crit_Hook(Game::Unit* atk, Game::Unit* tgt, uint16_t skill, uint32_t flags) {
    Hooks::Guard g;
    bool atkParty = Party(atk), tgtParty = Party(tgt);
    bool force = g_o.partyCrits && atkParty && !tgtParty;
    if (force) ++t_rollZero;
    int r = o_Crit(atk, tgt, skill, flags);
    if (force) --t_rollZero;
    // A party member hit by an enemy is never weak / critically hit, so never knocked down.
    if (g_o.noDowns && tgtParty && !atkParty && (r == 2 || r == 4)) r = 1;
    // Weakness override: the affinity hook already made the hit "weak"; nothing more here.
    return r;
}

uint32_t Rand_Hook() {
    Hooks::Guard g;
    // Only while a party attack's crit roll runs: 0 % 100 = 0 is below any crit chance.
    if (t_rollZero > 0) return 0;
    return o_Rand();
}

uint32_t Aff_Hook(Game::Unit* u, short elem) {
    Hooks::Guard g;
    uint32_t r = o_Aff(u, elem);
    if (!u) return r;
    if (Party(u)) {
        if (g_o.noInstakill && (elem == 6 || elem == 7)) return kNull;
        if (g_o.noAilments && elem >= 8 && elem <= 15) return kNull;
        if (g_o.noDowns && (r & kWeak)) return (r & ~kWeak & 0xFFFF0000u) | 100u;
        return r;
    }
    // An enemy's affinity is asked for when the party attacks it.
    if (g_o.weakness && elem >= 0 && elem <= 7 && !(r & kWeak)) {
        for (short e = 0; e <= 7; ++e) {
            if (e == 5) continue;  // Almighty is never a weakness
            uint32_t w = o_Aff(u, e);
            if (w & kWeak) return w;  // treated exactly as a hit with that element
        }
        if (g_o.weaknessForce) return (r & ~(kResist | kNull | 0x06000000u) & 0xFFFF0000u) | kWeak | 150u;
    }
    return r;
}

int Flee_Hook() {
    Hooks::Guard g;
    // The shadow waits instead: same script argument, AI_WAIT's own code.
    if (g_o.noFlee && g_wait) return g_wait();
    return o_Flee();
}

int Can_Hook(void* turn, short state) {
    Hooks::Guard g;
    int r = o_Can(turn, state);
    if (r || state != 5 || !g_o.unlimitedSwap || !turn) return r;
    // Lift only the "already changed this turn" condition; the rest of the game's rule holds:
    // the actor must be Yu (id 1), a real party member, with more than one Persona.
    auto* actor = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(turn) + 0x38);
    if (!actor || !Sig::Readable(reinterpret_cast<uintptr_t>(actor), 0xA8)) return r;
    if (actor[0xA2] != 0 || *reinterpret_cast<uint16_t*>(actor + 0xA4) != 1) return r;
    using Count_t = uint16_t (*)(int, int);
    uint16_t personas = reinterpret_cast<Count_t>(Addr::PersonaCount)(1, 0);
    return personas > 1 ? 1 : r;
}

uint64_t Known_Hook(uint16_t enemy, uint16_t elem) {
    Hooks::Guard g;
    if (g_o.revealAffinities) return 1;
    return o_Known(enemy, elem);
}

}  // namespace

Options& Opts() { return g_o; }

void SaveOptions() {
    Settings::SetInt("battlefx", "noDowns", g_o.noDowns);
    Settings::SetInt("battlefx", "noAilments", g_o.noAilments);
    Settings::SetInt("battlefx", "noInstakill", g_o.noInstakill);
    Settings::SetInt("battlefx", "noDebuffs", g_o.noDebuffs);
    Settings::SetInt("battlefx", "enemiesMiss", g_o.enemiesMiss);
    Settings::SetInt("battlefx", "partyHits", g_o.partyHits);
    Settings::SetInt("battlefx", "partyCrits", g_o.partyCrits);
    Settings::SetInt("battlefx", "noFlee", g_o.noFlee);
    Settings::SetInt("battlefx", "weakness", g_o.weakness);
    Settings::SetInt("battlefx", "weaknessForce", g_o.weaknessForce);
    Settings::SetInt("battlefx", "unlimitedSwap", g_o.unlimitedSwap);
    Settings::SetInt("battlefx", "revealAffinities", g_o.revealAffinities);
}

bool InstallHooks() {
    auto b = [](const char* k) { return Settings::GetInt("battlefx", k, 0) != 0; };
    g_o.noDowns = b("noDowns");
    g_o.noAilments = b("noAilments");
    g_o.noInstakill = b("noInstakill");
    g_o.noDebuffs = b("noDebuffs");
    g_o.enemiesMiss = b("enemiesMiss");
    g_o.partyHits = b("partyHits");
    g_o.partyCrits = b("partyCrits");
    g_o.noFlee = b("noFlee");
    g_o.weakness = b("weakness");
    g_o.weaknessForce = b("weaknessForce");
    g_o.unlimitedSwap = b("unlimitedSwap");
    g_o.revealAffinities = b("revealAffinities");
    if (!HostCheck::GroupOk("battlefx")) {
        LOGW("battle overrides: address checks failed; unavailable");
        return false;
    }
    auto h = [](const char* n, uintptr_t a, auto det, auto* orig) {
        return Hooks::Create(n, reinterpret_cast<void*>(a), det, orig);
    };
    bool ok = h("HitResolver", Addr::HitResolver, &Hit_Hook, &o_Hit);
    ok &= h("CritResolver", Addr::CritResolver, &Crit_Hook, &o_Crit);
    ok &= h("GetAffinityResult", Addr::GetAffinityResult, &Aff_Hook, &o_Aff);
    ok &= h("rand", Addr::Rand, &Rand_Hook, &o_Rand);
    ok &= h("AI_RUN_AWAY", Addr::AiRunAway, &Flee_Hook, &o_Flee);
    ok &= h("CanChangeToState", Addr::CanChangeToState, &Can_Hook, &o_Can);
    ok &= h("GetAffinityKnown", Addr::GetAffinityKnown, &Known_Hook, &o_Known);
    g_wait = reinterpret_cast<Ai_t>(Addr::AiWait);
    return ok;
}

void Tick() {
    if (!(g_o.noDebuffs || g_o.noAilments || g_o.noDowns)) return;
    if (!Sig::Readable(Addr::pBattleMgr, 8) || !*reinterpret_cast<const uintptr_t*>(Addr::pBattleMgr)) return;
    auto* units = reinterpret_cast<Game::Unit*>(Addr::SaveBlock + Game::Off::Units);
    for (int i = 0; i < Game::kUnits; ++i) {
        Game::Unit& u = units[i];
        if (i > 0 && !(u.flags & 1)) continue;
        if (u.ailments & 0x80000) continue;  // knocked out: leave it to the game
        uint32_t clear = 0;
        if (g_o.noAilments) clear |= 0xFF;      // dizzy, rage, fear, silence, poison, ...
        if (g_o.noDowns) clear |= 0x100000;     // down
        if (u.ailments & clear) u.ailments &= ~clear;
        if (g_o.noDebuffs) {
            auto* b = reinterpret_cast<uint8_t*>(&u) + 0x1C;
            for (int k = 0; k < 4; ++k) {
                uint8_t hi = b[k] >> 4, lo = b[k] & 0xF;
                if (hi & 8) hi = 0;  // a penalty stage (bit 3 set): back to neutral
                if (lo & 8) lo = 0;
                b[k] = uint8_t(hi << 4 | lo);
            }
        }
    }
}

}  // namespace BattleFx
