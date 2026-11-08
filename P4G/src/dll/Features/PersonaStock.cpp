#include "Features/PersonaStock.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

#include "Engine/Addresses.h"
#include "Engine/FlowCaller.h"
#include "Engine/GameState.h"
#include "Engine/GameThread.h"
#include "Engine/HostCheck.h"
#include "Engine/Names.h"
#include "Engine/SigScan.h"
#include "Features/Journal.h"
#include "Features/Spoilers.h"
#include "Hooks/Hook.h"
#include "Util/Settings.h"
#include "Util/Log.h"

namespace PersonaStock {

namespace {

using Game::Persona;

// Persona table flags (u16 at +0 of each 0xE-byte row), read from how the game uses them:
constexpr uint16_t kFlagSpecial = 0x02;   // special fusion
constexpr uint16_t kFlagUnused = 0x04;    // unused entries (an unobtainable Izanami / Okami)
constexpr uint16_t kFlagStory = 0x08;     // story / not Yu's (also the unused id 0)
constexpr uint16_t kFlagParty = 0x10;     // a party member's Persona
constexpr uint16_t kFlagNgPlus = 0x20;    // New Game+ (Izanagi-no-Okami); also unused placeholders
constexpr uint16_t kFlagUltimate = 0x40;  // needs the arcana's link at max

uint8_t* Block() { return reinterpret_cast<uint8_t*>(Addr::SaveBlock); }
Persona* Stock() { return reinterpret_cast<Persona*>(Block() + Game::Off::Stock); }
int16_t& Equipped() { return *reinterpret_cast<int16_t*>(Block() + Game::Off::EquippedSlot); }
uint32_t& Yen() { return *reinterpret_cast<uint32_t*>(Block() + Game::Off::Yen); }

Persona* Compendium() {
    if (!Sig::Readable(Addr::pCompendium, 8)) return nullptr;
    auto p = *reinterpret_cast<uintptr_t*>(Addr::pCompendium);
    return p ? reinterpret_cast<Persona*>(p) : nullptr;
}

uint16_t TableFlags(uint16_t id) {
    if (!Sig::Readable(Addr::pPersonaTable, 8)) return 0xFFFF;
    auto t = *reinterpret_cast<uintptr_t*>(Addr::pPersonaTable);
    if (!t || id >= 256 || !Sig::Readable(t + id * 0xE, 2)) return 0xFFFF;
    return *reinterpret_cast<uint16_t*>(t + id * 0xE);
}

template <typename F>
F At(uintptr_t a) {
    return reinterpret_cast<F>(a);
}

void NativeCreate(Persona* out, uint16_t id) {
    std::memset(out, 0, sizeof *out);
    At<void (*)(Persona*, uint16_t)>(Addr::CreatePersona)(out, id);
}
int NativeAdd(const Persona* p) { return At<int (*)(const Persona*)>(Addr::AddPersonaCopy)(p); }
int NativeRemove(uint16_t slot) { return At<int (*)(uint16_t)>(Addr::RemoveStockSlot)(slot); }
void NativeRegister(const Persona* p) { At<void (*)(const Persona*)>(Addr::CompendiumRegister)(p); }

std::string Fmt(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return buf;
}

// The stock must not change under the game's own menus or a battle.
const char* Busy() {
    const auto& s = Game::Current();
    if (!s.valid) return "No save is loaded.";
    if (s.inBattle) return "Not during a battle.";
    if (s.campMenu) return "Close the game's menu first.";
    if (Flow::YieldingBusy()) return "Waiting for the last change to finish.";
    return nullptr;
}

// Remove the Persona with this id again (undo of a give / summon), if it is still there and
// isn't the one equipped.
void RemoveById(uint16_t id) {
    int slot = SlotOf(id);
    if (slot < 0 || slot == Equipped() || Count() <= 1) return;
    NativeRemove(uint16_t(slot));
}

}  // namespace

bool Available() { return HostCheck::GroupOk("stock") && HostCheck::GroupOk("core"); }

Info Describe(uint16_t id) {
    // The Persona table and names never change once loaded, so they're read once.
    static Info cache[256];
    static bool filled = false;
    if (id == 0 || id >= 256) return {};
    if (!filled) {
        if (TableFlags(1) == 0xFFFF || Names::Persona(1).empty()) return {};  // not loaded yet
        for (uint16_t i = 1; i < 256; ++i) {
            Info& d = cache[i];
            uint16_t f = TableFlags(i);
            if (f == 0xFFFF) continue;
            d.exists = !Names::Persona(i).empty();
            d.special = f & kFlagSpecial;
            d.ultimate = f & kFlagUltimate;
            d.ngPlus = (f & kFlagNgPlus) && d.exists;
            d.yours = d.exists && !(f & (kFlagStory | kFlagParty | kFlagUnused));
            d.arcana = Game::PersonaArcana(i);
            d.baseLevel = Game::PersonaBaseLevel(i);
        }
        filled = true;
    }
    return cache[id];
}

int ArcanaLink(uint8_t arcana) {
    switch (arcana) {
        case 25: return 31;  // Jester -> Adachi
        case 27: return 33;  // Aeon -> Marie
    }
    return 0;
}

bool ArcanaVisible(uint8_t arcana) {
    int link = ArcanaLink(arcana);
    if (!link) return true;
    const Game::Link* e = Game::Current().FindLink(uint16_t(link));
    return Spoilers::Visible(Spoilers::Kind::Link, link, e && e->rank);
}

namespace {
bool FillCandidate(uint16_t id) {
    Info i = Describe(id);
    return i.yours && !i.ngPlus && ArcanaVisible(i.arcana);
}
}  // namespace

int CompendiumGaps() {
    int n = 0;
    for (uint16_t id = 1; id < 256; ++id)
        if (FillCandidate(id) && !Registered(id)) ++n;
    return n;
}

int Capacity() {
    if (!Available()) return Game::kStock;
    int c = At<int (*)()>(Addr::StockCapacity)() & 0xFFFF;
    return (c >= 1 && c <= Game::kStock) ? c : Game::kStock;
}

int Count() {
    int n = 0;
    for (int i = 0; i < Game::kStock; ++i) n += Stock()[i].valid & 1;
    return n;
}

int SlotOf(uint16_t id) {
    for (int i = 0; i < Game::kStock; ++i)
        if ((Stock()[i].valid & 1) && Stock()[i].id == id) return i;
    return -1;
}

const Persona* CompendiumEntry(uint16_t id) {
    Persona* c = Compendium();
    if (!c || id >= 256) return nullptr;
    return (c[id].valid & 1) ? &c[id] : nullptr;
}

bool Registered(uint16_t id) { return CompendiumEntry(id) != nullptr; }

int Price(uint16_t id) {
    const Persona* p = CompendiumEntry(id);
    if (!p || !Available()) return -1;
    return At<int (*)(const Persona*)>(Addr::SummonPrice)(p);
}

const char* WhyNotGive(uint16_t id) {
    if (!Available()) return "Unavailable on this game build.";
    if (const char* b = Busy()) return b;
    Info i = Describe(id);
    if (!i.yours) return "Only Yu's own Personas can be given.";
    if (SlotOf(id) >= 0) return "Already in Yu's stock (the game allows one of each).";
    if (Count() >= Capacity()) return "The stock is full. Remove one first.";
    return nullptr;
}

const char* WhyNotRemove(int slot) {
    if (!Available()) return "Unavailable on this game build.";
    if (const char* b = Busy()) return b;
    if (slot < 0 || slot >= Game::kStock || !(Stock()[slot].valid & 1)) return "That slot is empty.";
    if (slot == Equipped()) return "That's the equipped Persona. Switch to another one in the game's menu first.";
    if (Count() <= 1) return "Yu always keeps at least one Persona.";
    return nullptr;
}

const char* WhyNotSummon(uint16_t id, bool pay) {
    if (!Available()) return "Unavailable on this game build.";
    if (const char* b = Busy()) return b;
    if (!Registered(id)) return "Not registered in the compendium.";
    if (SlotOf(id) >= 0) return "Already in Yu's stock.";
    if (Count() >= Capacity()) return "The stock is full. Remove one first.";
    if (pay && Price(id) > int(Yen())) return "Not enough money for the fee.";
    return nullptr;
}

void Give(uint16_t id) {
    GameThread::Post([id] {
        if (WhyNotGive(id)) return;
        bool wasRegistered = Registered(id);
        Persona saved{};
        if (Persona* c = Compendium()) saved = c[id];
        Persona p;
        NativeCreate(&p, id);
        if (p.id != id || !NativeAdd(&p)) {
            LOGW("give persona %u: the game refused (stock full?)", id);
            return;
        }
        std::string name = Names::PersonaOr(id);
        Journal::Record(Fmt("Gave %s (Lv %u)%s", name.c_str(), p.level,
                            wasRegistered ? "" : " — also registered in the compendium"),
                        [id, wasRegistered, saved] {
                            RemoveById(id);
                            if (!wasRegistered)
                                if (Persona* c = Compendium()) c[id] = saved;
                        });
    });
}

void Remove(int slot) {
    GameThread::Post([slot] {
        if (WhyNotRemove(slot)) return;
        Persona saved = Stock()[slot];
        if (!NativeRemove(uint16_t(slot))) return;
        Journal::Record(Fmt("Removed %s (Lv %u) from the stock", Names::PersonaOr(saved.id).c_str(), saved.level),
                        [saved] {
                            if (SlotOf(saved.id) < 0 && Count() < Capacity()) NativeAdd(&saved);
                        });
    });
}

void Summon(uint16_t id, bool pay) {
    GameThread::Post([id, pay] {
        if (WhyNotSummon(id, pay)) return;
        Persona copy = *CompendiumEntry(id);
        int fee = pay ? Price(id) : 0;
        if (fee < 0) fee = 0;
        if (!NativeAdd(&copy)) return;
        if (fee) Yen() -= uint32_t(fee);
        Journal::Record(Fmt("Summoned %s from the compendium%s", Names::PersonaOr(id).c_str(),
                            fee ? Fmt(" for ¥%d", fee).c_str() : " (free)"),
                        [id, fee] {
                            RemoveById(id);
                            if (fee) Yen() += uint32_t(fee);
                        });
    });
}

void Register(int slot) {
    GameThread::Post([slot] {
        if (!Available() || Busy() || slot < 0 || slot >= Game::kStock) return;
        Persona p = Stock()[slot];
        if (!(p.valid & 1)) return;
        Persona* c = Compendium();
        if (!c) return;
        Persona old = c[p.id];
        NativeRegister(&p);
        Journal::Record(Fmt("Registered %s (Lv %u) in the compendium", Names::PersonaOr(p.id).c_str(), p.level),
                        [old, id = p.id] {
                            if (Persona* c2 = Compendium()) c2[id] = old;
                        });
    });
}

void FillCompendium() {
    GameThread::Post([] {
        if (!Available() || Busy()) return;
        Persona* c = Compendium();
        if (!c) return;
        std::vector<std::pair<uint16_t, Persona>> before;
        for (uint16_t id = 1; id < 256; ++id) {
            if (!FillCandidate(id) || (c[id].valid & 1)) continue;
            Persona p;
            NativeCreate(&p, id);
            if (p.id != id) continue;
            // As the game's add-to-stock does before registering: keep bits 2-3, mark valid.
            *reinterpret_cast<uint16_t*>(&p) = uint16_t((*reinterpret_cast<uint16_t*>(&p) & 0xC) | 1);
            before.push_back({id, c[id]});
            NativeRegister(&p);
        }
        if (before.empty()) return;
        Journal::Record(Fmt("Registered %zu Personas in the compendium (base forms)", before.size()), [before] {
            if (Persona* c2 = Compendium())
                for (const auto& e : before) c2[e.first] = e.second;
        });
    });
}

// --- Velvet Room rule override ------------------------------------------------------------------
namespace {
using Unlocked_t = int (*)(uint16_t);
Unlocked_t o_Unlocked = nullptr;
std::atomic<bool> g_ultimates{false};

int Unlocked_Hook(uint16_t id) {
    Hooks::Guard g;
    if (g_ultimates.load(std::memory_order_relaxed)) return 1;
    return o_Unlocked(id);
}
}  // namespace

bool UnlockOverrideAvailable() { return HostCheck::GroupOk("fusion") && o_Unlocked != nullptr; }
bool UltimatesUnlocked() { return g_ultimates.load(); }

void SetUltimatesUnlocked(bool on) {
    g_ultimates = on;
    Settings::SetInt("fusion", "ultimates", on ? 1 : 0);
    if (UnlockOverrideAvailable())  // the game's own answer, through the hook (Loki, Fool's ultimate)
        LOGI("fusion: ultimates %s; Loki unlocked = %d", on ? "unlocked" : "normal",
             reinterpret_cast<int (*)(uint16_t)>(Addr::PersonaUnlocked)(4));
}

bool InstallHooks() {
    g_ultimates = Settings::GetInt("fusion", "ultimates", 0) != 0;
    if (!HostCheck::GroupOk("fusion")) return false;
    return Hooks::Create("PersonaUnlocked", reinterpret_cast<void*>(Addr::PersonaUnlocked), &Unlocked_Hook,
                         &o_Unlocked);
}

int UnlockFlagCount() {
    if (!HostCheck::GroupOk("fusion") || !Sig::Readable(Addr::UnlockCount, 4)) return 0;
    uint32_t n = *reinterpret_cast<uint32_t*>(Addr::UnlockCount);
    return n <= 256 ? int(n) : 0;
}

bool UnlockEntry(int i, int* flag, uint16_t* persona) {
    if (i < 0 || i >= UnlockFlagCount() || !Sig::Readable(Addr::pUnlockTable, 8)) return false;
    auto t = *reinterpret_cast<uintptr_t*>(Addr::pUnlockTable);
    if (!t || !Sig::Readable(t + i * 8, 8)) return false;
    *flag = *reinterpret_cast<int32_t*>(t + i * 8);
    *persona = *reinterpret_cast<uint16_t*>(t + i * 8 + 4);
    return true;
}

}  // namespace PersonaStock
