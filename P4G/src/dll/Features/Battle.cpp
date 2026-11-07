#include "Features/Battle.h"

#include "Engine/Addresses.h"
#include "Engine/HostCheck.h"
#include "Engine/Natives.h"
#include "Engine/SigScan.h"
#include "Hooks/Hook.h"
#include "Util/Log.h"
#include "Util/Settings.h"

namespace Battle {

namespace {

Options g_opts;

using Add_t = void (*)(Game::Unit*, int);
Add_t o_AddHP = nullptr;
Add_t o_AddSP = nullptr;

uintptr_t UnitsBase() { return Addr::SaveBlock + Game::Off::Units; }

bool InScope(const Game::Unit* u) {
    if (!IsPartyUnit(u)) return false;
    if (g_opts.scope == Scope::YuOnly)
        return reinterpret_cast<uintptr_t>(u) == UnitsBase();  // index 0 = protagonist
    return true;
}

void AddHP_Hook(Game::Unit* u, int delta) {
    Hooks::Guard g;
    if (delta < 0 && !Natives::g_trainerCall && g_opts.hp != HpMode::Off && u && InScope(u)) {
        if (g_opts.hp == HpMode::Lock) return;  // the hit doesn't land
        if (u->hp + delta < 1) delta = 1 - int(u->hp);
        if (delta >= 0) return;
    }
    o_AddHP(u, delta);
}

void AddSP_Hook(Game::Unit* u, int delta) {
    Hooks::Guard g;
    if (delta < 0 && !Natives::g_trainerCall && g_opts.sp && u && InScope(u)) return;
    o_AddSP(u, delta);
}

}  // namespace

Options& Opts() { return g_opts; }

void LoadOptions() {
    g_opts.hp = HpMode(Settings::GetInt("battle", "hp", 0));
    g_opts.sp = Settings::GetInt("battle", "sp", 0) != 0;
    g_opts.scope = Scope(Settings::GetInt("battle", "scope", 0));
}

void SaveOptions() {
    Settings::SetInt("battle", "hp", int(g_opts.hp));
    Settings::SetInt("battle", "sp", g_opts.sp ? 1 : 0);
    Settings::SetInt("battle", "scope", int(g_opts.scope));
}

bool InstallHooks() {
    LoadOptions();
    if (!HostCheck::GroupOk("battle")) {
        LOGW("battle: address checks failed; survival options unavailable");
        return false;
    }
    bool ok = Hooks::Create("AddHP", reinterpret_cast<void*>(Addr::AddHP), &AddHP_Hook, &o_AddHP);
    ok &= Hooks::Create("AddSP", reinterpret_cast<void*>(Addr::AddSP), &AddSP_Hook, &o_AddSP);
    return ok;
}

bool IsPartyUnit(const Game::Unit* u) {
    auto a = reinterpret_cast<uintptr_t>(u);
    uintptr_t base = UnitsBase();
    if (a >= base && a < base + Game::kUnits * sizeof(Game::Unit))
        return (a - base) % sizeof(Game::Unit) == 0;
    // Scripted fights: the battle manager's party list (node +0xCF0 = unit, +0xD00 = next).
    if (!Sig::Readable(Addr::pBattleMgr, 8)) return false;
    uintptr_t bm = *reinterpret_cast<const uintptr_t*>(Addr::pBattleMgr);
    if (!bm || !Sig::Readable(bm + 0x1D0, 8)) return false;
    uintptr_t n = *reinterpret_cast<const uintptr_t*>(bm + 0x1D0);
    for (int i = 0; n && i < 16; ++i) {
        if (!Sig::Readable(n + 0xCF0, 0x18)) break;
        if (*reinterpret_cast<const uintptr_t*>(n + 0xCF0) == a) return true;
        n = *reinterpret_cast<const uintptr_t*>(n + 0xD00);
    }
    return false;
}

void RefillNow() {
    if (!HostCheck::GroupOk("battle")) return;
    auto* units = reinterpret_cast<Game::Unit*>(UnitsBase());
    int count = g_opts.scope == Scope::YuOnly ? 1 : Game::kUnits;
    for (int i = 0; i < count; ++i) {
        Game::Unit* u = &units[i];
        if (i > 0 && !(u->flags & 1)) continue;  // only members currently in the party
        uint16_t mh = Natives::GetMaxHP(u), ms = Natives::GetMaxSP(u);
        if (mh && u->hp < mh) Natives::AddHP(u, mh - u->hp);
        if (ms && u->sp < ms) Natives::AddSP(u, ms - u->sp);
    }
}

}  // namespace Battle
