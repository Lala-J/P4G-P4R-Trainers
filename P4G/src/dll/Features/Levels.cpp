#include "Features/Levels.h"

#include <cstdio>
#include <cstring>

#include "Engine/Addresses.h"
#include "Engine/GameState.h"
#include "Engine/GameThread.h"
#include "Engine/HostCheck.h"
#include "Engine/Names.h"
#include "Features/Journal.h"
#include "Hooks/Hook.h"
#include "Util/Log.h"
#include "Util/Settings.h"

namespace Levels {

namespace {

Options g_o;

using Gen_t = void (*)(Game::Persona*, uint8_t* changes, uint32_t gained);
using Apply_t = void (*)(Game::Persona*, uint8_t* changes);
using Req_t = uint32_t (*)(Game::Persona*, uint16_t level);
using Member_t = Game::Persona* (*)(uint16_t charId);
using Calc_t = uint32_t (*)(uint32_t, void*);
using Give_t = void (*)(uint8_t* results);

Calc_t o_Calc = nullptr;
Give_t o_Give = nullptr;

Game::Persona* Resolve(const Target& t) {
    if (!t.member) {
        if (t.index < 0 || t.index >= Game::kStock) return nullptr;
        auto* p = reinterpret_cast<Game::Persona*>(Addr::SaveBlock + Game::Off::Stock) + t.index;
        return p->valid ? p : nullptr;
    }
    if (t.index < 2 || t.index > 8) return nullptr;
    auto* p = reinterpret_cast<Member_t>(Addr::GetPartyMemberPersona)(uint16_t(t.index));
    return (p && p->id) ? p : nullptr;
}

// The battle-results sequence for one Persona. Returns levels gained.
int Grant(Game::Persona* p, uint32_t gained) {
    if (!p || p->level >= 99 || !gained) return 0;
    alignas(8) uint8_t changes[0x88];
    std::memset(changes, 0, sizeof changes);
    reinterpret_cast<Gen_t>(Addr::GenerateLevelUpPersona)(p, changes, gained);
    p->exp += gained;
    uint8_t before = p->level;
    if (changes[0]) reinterpret_cast<Apply_t>(Addr::ApplyLevelUp)(p, changes);
    return p->level - before;
}

uint32_t Calc_Hook(uint32_t level, void* info) {
    Hooks::Guard g;
    uint32_t r = o_Calc(level, info);
    if (g_o.expMultiplier > 1) {
        uint64_t m = uint64_t(r) * uint32_t(g_o.expMultiplier);
        r = m > 99999999u ? 99999999u : uint32_t(m);
    }
    return r;
}

// After the game hands out battle EXP to the active party, give benched members the same.
void Give_Hook(uint8_t* results) {
    Hooks::Guard g;
    o_Give(results);
    if (!g_o.benchShare || !results) return;
    auto* ids = reinterpret_cast<uint16_t*>(results + 0x69A);
    auto* gains = reinterpret_cast<uint32_t*>(results + 0x6A4);
    uint32_t best = 0;
    for (int i = 0; i < 4; ++i)
        if (ids[i] > 1 && gains[i] > best) best = gains[i];
    if (!best) return;
    const auto* flags = reinterpret_cast<const uint8_t*>(Addr::SaveBlock + Game::Off::Flags);
    static const int join[9] = {-1, -1, 48, 49, 50, 52, 51, 54, 53};
    for (int c = 2; c <= 8; ++c) {
        bool active = false;
        for (int i = 0; i < 4; ++i) active |= ids[i] == c;
        if (active || !((flags[join[c] >> 3] >> (join[c] & 7)) & 1)) continue;
        int lv = Grant(Resolve({true, c}), best);
        if (lv) LOGI("levels: benched member %d gained %u EXP and %d level(s)", c, best, lv);
    }
}

}  // namespace

bool Available() { return HostCheck::GroupOk("levels"); }

const char* TargetName(const Target& t, char* buf, int n) {
    static const char* names[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
    if (t.member) snprintf(buf, n, "%s's Persona", (t.index >= 1 && t.index <= 8) ? names[t.index] : "?");
    else {
        auto* p = reinterpret_cast<Game::Persona*>(Addr::SaveBlock + Game::Off::Stock) + t.index;
        snprintf(buf, n, "%s", Names::PersonaOr(p->id).c_str());
    }
    return buf;
}

uint32_t ExpForPersonaLevel(const Game::Persona* p, int level) {
    if (!Available() || !p || level < 1 || level > 99) return 0;
    return reinterpret_cast<Req_t>(Addr::PersonaReqExp)(const_cast<Game::Persona*>(p), uint16_t(level));
}

void AddExp(Target t, uint32_t exp) {
    GameThread::Post([t, exp] {
        if (!Available()) return;
        Game::Persona* p = Resolve(t);
        if (!p) return;
        uint8_t oldLv = p->level;
        uint32_t oldExp = p->exp;
        int lv = Grant(p, exp);
        char nm[48];
        char what[160];
        snprintf(what, sizeof what, "%s +%u EXP (Lv %u → %u)", TargetName(t, nm, sizeof nm), exp, oldLv, p->level);
        LOGI("levels: %s", what);
        (void)lv;
        (void)oldExp;
        // Level-ups are permanent game growth (stats, skills); only the EXP figure is undoable.
        Journal::Record(what, nullptr);
    });
}

void LevelUp(Target t, int levels) {
    GameThread::Post([t, levels] {
        if (!Available()) return;
        Game::Persona* p = Resolve(t);
        if (!p) return;
        uint8_t oldLv = p->level;
        for (int i = 0; i < levels && p->level < 99; ++i) {
            uint32_t need = ExpForPersonaLevel(p, p->level + 1);
            uint32_t gained = need > p->exp ? need - p->exp : 1;
            if (!Grant(p, gained)) break;
        }
        char nm[48], what[160];
        snprintf(what, sizeof what, "%s level %u → %u (the game's own level-up growth)",
                 TargetName(t, nm, sizeof nm), oldLv, p->level);
        LOGI("levels: %s", what);
        Journal::Record(what, nullptr);
    });
}

void SetYuLevel(int level) {
    if (level < 1 || level > 99) return;
    GameThread::Post([level] {
        auto* yu = reinterpret_cast<Game::Unit*>(Addr::SaveBlock + Game::Off::Units);
        uint32_t exp = Game::ExpForLevel(level);
        uint8_t oldLv = yu->level;
        uint32_t oldExp = yu->exp;
        if (oldLv == level) return;
        yu->level = uint8_t(level);
        yu->exp = exp;
        char what[96];
        snprintf(what, sizeof what, "Yu level %u → %d", oldLv, level);
        Journal::Record(what, [oldLv, oldExp] {
            auto* u = reinterpret_cast<Game::Unit*>(Addr::SaveBlock + Game::Off::Units);
            u->level = oldLv;
            u->exp = oldExp;
        });
    });
}

Options& Opts() { return g_o; }

void SaveOptions() {
    Settings::SetInt("levels", "mult", g_o.expMultiplier);
    Settings::SetInt("levels", "bench", g_o.benchShare ? 1 : 0);
}

bool InstallHooks() {
    g_o.expMultiplier = Settings::GetInt("levels", "mult", 1);
    if (g_o.expMultiplier < 1 || g_o.expMultiplier > 10) g_o.expMultiplier = 1;
    g_o.benchShare = Settings::GetInt("levels", "bench", 0) != 0;
    if (!Available()) return false;
    bool ok = Hooks::Create("CalculateGainedExp", reinterpret_cast<void*>(Addr::CalculateGainedExp), &Calc_Hook, &o_Calc);
    ok &= Hooks::Create("GivePartyMembersExp", reinterpret_cast<void*>(Addr::GivePartyMembersExp), &Give_Hook, &o_Give);
    return ok;
}

}  // namespace Levels
