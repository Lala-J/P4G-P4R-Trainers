#include "Features/FieldGuard.h"

#include <windows.h>
#include <atomic>
#include <cstdint>

#include "Engine/Addresses.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"
#include "Hooks/Hook.h"
#include "Util/Log.h"

namespace FieldGuard {

namespace {

// (object, room, float, ?, const float* position, float) -- the 3rd argument is a float (xmm2).
using Step_t = uint64_t (*)(void*, void*, float, void*, const float*, float);
Step_t o_Step = nullptr;
std::atomic<int> g_fired{0};
std::atomic<ULONGLONG> g_lastLog{0};

uintptr_t Follow(uintptr_t a, size_t off) {
    if (!a || !Sig::Readable(a + off, 8)) return 0;
    return *reinterpret_cast<uintptr_t*>(a + off);
}

bool LeaderPos(float* x, float* y, float* z) {
    uintptr_t pl = Follow(Follow(Follow(Follow(Addr::pFieldRoot, 0), 0x250), 0x48), 0x18);
    if (!pl || !Sig::Readable(pl + 0x360, 12)) return false;
    *x = *reinterpret_cast<float*>(pl + 0x360);
    *y = *reinterpret_cast<float*>(pl + 0x364);
    *z = *reinterpret_cast<float*>(pl + 0x368);
    return true;
}

uint64_t Step_Hook(void* object, void* room, float a3, void* a4, const float* a5, float a6) {
    Hooks::Guard g;
    if (room) return o_Step(object, room, a3, a4, a5, a6);
    ++g_fired;
    ULONGLONG now = GetTickCount64();
    if (now - g_lastLog.load() > 5000) {  // at most one line every 5 s
        g_lastLog = now;
        float x = 0, y = 0, z = 0;
        LeaderPos(&x, &y, &z);
        LOGW("field guard: an object stood outside every room (would have crashed the game); "
             "leader at x %.0f y %.0f z %.0f, cell (%d, %d); %d time(s) so far",
             x, y, z, int((z + 600) / 1200), int((x + 600) / 1200), g_fired.load());
    }
    return 0;  // what the step returns when a probe blocks the move
}

}  // namespace

bool InstallHooks() {
    if (!HostCheck::GroupOk("fieldguard")) return false;
    return Hooks::Create("FieldCollisionStep", reinterpret_cast<void*>(Addr::FieldCollisionStep), &Step_Hook,
                         &o_Step);
}

int Fired() { return g_fired.load(); }

bool LeaderOutsideRoom() {
    float x, y, z;
    if (!LeaderPos(&x, &y, &z) || !(x >= 0.f && z >= 0.f)) return false;
    int mi = int((x + 600.f) / 1200.f), ma = int((z + 600.f) / 1200.f);
    if (mi < 0 || mi >= 16 || ma < 0 || ma >= 24) return false;
    const uint8_t* c = reinterpret_cast<const uint8_t*>(Addr::MinimapGrid) + (ma * 16 + mi) * 16;
    return c[0] != 1 || *reinterpret_cast<const uint16_t*>(c + 2) == 0;
}

}  // namespace FieldGuard
