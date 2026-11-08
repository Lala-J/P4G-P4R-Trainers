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
    return 0;  // skip the step; the rescue in Tick() puts the party back on the floor
}

// --- Rescue ----------------------------------------------------------------------------------
// The back end of some staircases isn't solid: the party can walk through it into the wall cell
// behind and on into the void. Each frame we remember where the leader last stood on a room cell
// and, the moment it stands in a wall cell, put it back there (or, if there is no such point yet,
// in the middle of the nearest room cell).
uintptr_t g_player = 0;
bool g_haveGood = false;
float g_good[3] = {};
std::atomic<int> g_rescues{0};

const uint8_t* Cell(int ma, int mi) {
    return reinterpret_cast<const uint8_t*>(Addr::MinimapGrid) + (ma * 16 + mi) * 16;
}
bool RoomCell(int ma, int mi) {
    if (ma < 0 || ma >= 24 || mi < 0 || mi >= 16) return false;
    const uint8_t* c = Cell(ma, mi);
    return c[0] == 1 && *reinterpret_cast<const uint16_t*>(c + 2) != 0;
}

}  // namespace

void Tick() {
    if (!HostCheck::GroupOk("fieldguard") || !Sig::Readable(Addr::pDungeonTask, 8) ||
        !*reinterpret_cast<const uintptr_t*>(Addr::pDungeonTask)) {
        g_player = 0;
        g_haveGood = false;
        return;
    }
    if (Sig::Readable(Addr::InBattle, 1) && *reinterpret_cast<const uint8_t*>(Addr::InBattle)) return;
    uintptr_t pl = Follow(Follow(Follow(Follow(Addr::pFieldRoot, 0), 0x250), 0x48), 0x18);
    if (!pl || !Sig::Readable(pl + 0x360, 12)) return;
    float* pos = reinterpret_cast<float*>(pl + 0x360);
    if (pl != g_player) {  // new floor / new field object
        g_player = pl;
        g_haveGood = false;
    }
    float x = pos[0], z = pos[2];
    if (!(x >= -600.f && z >= -600.f && x < 16 * 1200.f && z < 24 * 1200.f)) return;
    int mi = int((x + 600.f) / 1200.f), ma = int((z + 600.f) / 1200.f);
    if (RoomCell(ma, mi)) {
        g_haveGood = true;
        g_good[0] = pos[0];
        g_good[1] = pos[1];
        g_good[2] = pos[2];
        return;
    }
    if (Cell(ma, mi)[0] != 2) return;  // blank cells (fixed floors) are normal ground
    float to[3];
    if (g_haveGood) {
        to[0] = g_good[0], to[1] = g_good[1], to[2] = g_good[2];
    } else {  // nearest room cell's centre
        int best = -1, bd = 1 << 30;
        for (int a = 0; a < 24; ++a)
            for (int b = 0; b < 16; ++b)
                if (RoomCell(a, b)) {
                    int d = (a - ma) * (a - ma) + (b - mi) * (b - mi);
                    if (d < bd) bd = d, best = a * 16 + b;
                }
        if (best < 0) return;
        to[0] = (best % 16) * 1200.f, to[1] = 0.f, to[2] = (best / 16) * 1200.f;
    }
    pos[0] = to[0], pos[1] = to[1], pos[2] = to[2];
    ++g_rescues;
    static ULONGLONG lastLog = 0;
    ULONGLONG now = GetTickCount64();
    if (now - lastLog > 5000) {  // pushing into it keeps triggering every frame
        lastLog = now;
        LOGW("field guard: the party walked out of the floor at x %.0f z %.0f (cell %d, %d); moved back to "
             "x %.0f z %.0f (%d time(s) so far)", x, z, ma, mi, to[0], to[2], g_rescues.load());
    }
}

int Rescues() { return g_rescues.load(); }


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
    // Only a generated floor's wall cell counts. Fixed story floors leave the grid blank (0), and a
    // blank cell there is perfectly normal ground.
    const uint8_t* c = reinterpret_cast<const uint8_t*>(Addr::MinimapGrid) + (ma * 16 + mi) * 16;
    return c[0] == 2;
}

}  // namespace FieldGuard
