#include "Features/Trace.h"

#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "Engine/Addresses.h"
#include "Engine/FlowCaller.h"
#include "Engine/GameState.h"
#include "Engine/GameTypes.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"
#include "Features/FieldQol.h"
#include "Hooks/Hook.h"
#include "Util/Log.h"
#include "Util/Paths.h"

namespace Trace {

namespace {

bool g_on = false;
uint8_t g_prevFlags[Game::kFlagBytes];
uint32_t g_prevCounters[Game::kCounters];
bool g_havePrev = false;
int g_changes[Game::kCounters] = {};  // counters that change constantly (timers) get muted
uintptr_t g_prevBattle = 0;
int g_budget = 0;  // log lines left this battle for the resolver hooks

using Aff_t = uint32_t (*)(void*, short);
using Known_t = uint64_t (*)(uint16_t, uint16_t);
Aff_t o_Aff = nullptr;
Known_t o_Known = nullptr;

uint32_t Aff_Hook(void* unit, short element) {
    Hooks::Guard g;
    uint32_t r = o_Aff(unit, element);
    if (g_budget > 0) {
        --g_budget;
        LOGI("trace: GetAffinityResult(unit %p, elem %d) = 0x%08X  from 0x%llX", unit, element, r,
             (unsigned long long)_ReturnAddress());
    }
    return r;
}

uint64_t Known_Hook(uint16_t enemy, uint16_t element) {
    Hooks::Guard g;
    uint64_t r = o_Known(enemy, element);
    if (g_budget > 0) {
        --g_budget;
        LOGI("trace: GetAffinityKnown(enemy %u, elem %u) = %llu", enemy, element, (unsigned long long)r);
    }
    return r;
}

uintptr_t Ptr(uintptr_t a) {
    return Sig::Readable(a, 8) ? *reinterpret_cast<const uintptr_t*>(a) : 0;
}

void DumpBattle(uintptr_t bm) {
    LOGI("trace: battle start, manager %p", (void*)bm);
    for (int side = 0; side < 2; ++side) {
        uintptr_t n = Ptr(bm + (side ? 0x1D8 : 0x1D0));
        for (int i = 0; n && i < 12; ++i) {
            uintptr_t unit = Ptr(n + 0xCF0);
            uint16_t id = Sig::Readable(unit, 4) ? *reinterpret_cast<const uint16_t*>(unit + 2) : 0;
            uint16_t hp = Sig::Readable(unit, 0x10) ? *reinterpret_cast<const uint16_t*>(unit + 8) : 0;
            uint32_t flags = Sig::Readable(n + 0x9C, 4) ? *reinterpret_cast<const uint32_t*>(n + 0x9C) : 0;
            LOGI("trace:   %s node %p unit %p id %u hp %u nodeflags 0x%X", side ? "enemy" : "party",
                 (void*)n, (void*)unit, id, hp, flags);
            n = Ptr(n + 0xD00);
        }
    }
}

// One-more research: capture battle state around a party attack and diff it later.
struct Cap {
    bool active = false;
    int result = 0;
    int frames = 0;
    uintptr_t bm = 0, node = 0;
    uint8_t bmBefore[0x1000];
    uint8_t nodeBefore[0xD10];
} g_cap;

uintptr_t NodeOf(uintptr_t bm, uintptr_t unit) {
    uintptr_t n = Ptr(bm + 0x1D0);
    for (int i = 0; n && i < 12; ++i) {
        if (Ptr(n + 0xCF0) == unit) return n;
        n = Ptr(n + 0xD00);
    }
    return 0;
}

void DiffLog(const char* what, const uint8_t* a, const uint8_t* b, size_t n) {
    int lines = 0;
    for (size_t i = 0; i < n && lines < 48; ++i)
        if (a[i] != b[i]) {
            LOGI("trace: 1more %s +0x%03zX: %02X -> %02X", what, i, a[i], b[i]);
            ++lines;
        }
}

}  // namespace

void OnPartyResolve(void* atk, int result) {
    if (!g_on || g_cap.active) return;
    uintptr_t bm = Ptr(Addr::pBattleMgr);
    if (!bm) return;
    uintptr_t node = NodeOf(bm, reinterpret_cast<uintptr_t>(atk));
    if (!node || !Sig::Readable(bm, 0x1000) || !Sig::Readable(node, 0xD10)) return;
    g_cap.active = true;
    g_cap.result = result;
    g_cap.frames = 90;
    g_cap.bm = bm;
    g_cap.node = node;
    std::memcpy(g_cap.bmBefore, reinterpret_cast<void*>(bm), 0x1000);
    std::memcpy(g_cap.nodeBefore, reinterpret_cast<void*>(node), 0xD10);
    LOGI("trace: 1more capture start, result %d, node %p", result, (void*)node);
}

bool Enabled() { return g_on; }

bool InstallHooks() {
    g_on = GetFileAttributesW((Paths::DataDir() + L"\\debug-trace").c_str()) != INVALID_FILE_ATTRIBUTES;
    if (!g_on || !HostCheck::Get().supported) return false;
    LOGI("trace: enabled");
    // (Affinity functions are hooked by BattleFx; one hook per function.)
    (void)&Aff_Hook;
    (void)&Known_Hook;
    return true;
}

void FlowSelfCheck() {
    // Compare script-function results with what the trainer reads directly.
    auto r = Flow::Call(Flow::Fn::GET_SL_LEVEL, {7});  // Yosuke
    const auto* links = reinterpret_cast<const Game::Link*>(Addr::SaveBlock + Game::Off::Links);
    int direct = -1;
    for (int i = 0; i < Game::kLinks; ++i)
        if (links[i].id == 7) direct = links[i].rank;
    LOGI("trace: flow GET_SL_LEVEL(7) ok=%d done=%d value=%lld, direct rank %d", r.ok, r.done,
         (long long)r.value, direct);
    for (int flag : {35, 36, 48}) {
        auto b = Flow::Call(Flow::Fn::BIT_CHK, {flag});
        const auto* f = reinterpret_cast<const uint8_t*>(Addr::SaveBlock + Game::Off::Flags);
        LOGI("trace: flow BIT_CHK(%d) ok=%d value=%lld, direct %d", flag, b.ok, (long long)b.value,
             (f[flag >> 3] >> (flag & 7)) & 1);
    }
    auto c = Flow::Call(Flow::Fn::GET_CNT, {16});
    const auto* cnt = reinterpret_cast<const uint32_t*>(Addr::SaveBlock + Game::Off::Counters);
    LOGI("trace: flow GET_CNT(16) ok=%d value=%lld, direct %u", c.ok, (long long)c.value, cnt[16]);
}

void Tick() {
    if (!g_on || !HostCheck::GroupOk("core")) return;
    static bool checked = false;
    if (!checked) {
        checked = true;
        FlowSelfCheck();
    }
    const auto* flags = reinterpret_cast<const uint8_t*>(Addr::SaveBlock + Game::Off::Flags);
    const auto* counters = reinterpret_cast<const uint32_t*>(Addr::SaveBlock + Game::Off::Counters);
    if (g_havePrev) {
        int lines = 0;
        for (int b = 0; b < Game::kFlagBytes && lines < 40; ++b) {
            uint8_t d = flags[b] ^ g_prevFlags[b];
            for (int bit = 0; d && bit < 8; ++bit, d >>= 1)
                if (d & 1) {
                    int n = b * 8 + bit;
                    LOGI("trace: flag %d %s", n, (flags[b] >> bit) & 1 ? "ON" : "off");
                    ++lines;
                }
        }
        for (int i = 0; i < Game::kCounters && lines < 60; ++i)
            if (counters[i] != g_prevCounters[i] && ++g_changes[i] <= 20) {
                LOGI("trace: counter %d %u -> %u%s", i, g_prevCounters[i], counters[i],
                     g_changes[i] == 20 ? " (changes constantly; muted)" : "");
                ++lines;
            }
    }
    std::memcpy(g_prevFlags, flags, sizeof g_prevFlags);
    std::memcpy(g_prevCounters, counters, sizeof g_prevCounters);
    g_havePrev = true;

    if (g_cap.active && --g_cap.frames <= 0) {
        g_cap.active = false;
        if (Ptr(Addr::pBattleMgr) == g_cap.bm && Sig::Readable(g_cap.node, 0xD10)) {
            LOGI("trace: 1more capture end, result %d", g_cap.result);
            DiffLog("bm", g_cap.bmBefore, reinterpret_cast<const uint8_t*>(g_cap.bm), 0x1000);
            DiffLog("node", g_cap.nodeBefore, reinterpret_cast<const uint8_t*>(g_cap.node), 0xD10);
        }
    }

    uintptr_t bm = Ptr(Addr::pBattleMgr);
    if (bm && !g_prevBattle) {
        g_budget = 400;
        DumpBattle(bm);
    } else if (!bm && g_prevBattle) {
        LOGI("trace: battle end");
    }
    g_prevBattle = bm;
}

}  // namespace Trace
