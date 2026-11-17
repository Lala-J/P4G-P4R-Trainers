#include "Engine/GameState.h"

#include <cstring>

#include "Engine/Addresses.h"
#include "Engine/FlowCaller.h"
#include "Engine/GameThread.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"
#include "Util/Log.h"

namespace Game {

namespace {

Snapshot g_snap;
uint64_t g_frame = 0;

template <typename T>
void Copy(T& dst, uintptr_t src) {
    std::memcpy(&dst, reinterpret_cast<const void*>(src), sizeof(T));
}

template <typename T>
T Read(uintptr_t a) {
    T v{};
    std::memcpy(&v, reinterpret_cast<const void*>(a), sizeof(T));
    return v;
}

uintptr_t Deref(uintptr_t cell) {
    if (!Sig::Readable(cell, 8)) return 0;
    uintptr_t v = Read<uintptr_t>(cell);
    return (v && Sig::Readable(v, 8)) ? v : 0;
}

// The weather is the game's answer to GET_WEATHER, the same call the town's scripts make (Aiya's
// rainy-day menu, the bug spot). Asked again whenever the day or time slot changes, and every
// ten seconds or so in case an event changes it; never in battle or without a save.
void CaptureWeather(Snapshot& s) {
    static int lastDay = -1, lastTime = -1;
    static uint64_t lastFrame = 0;
    if (!s.valid) {
        s.weather = -1;
        lastDay = -1;
        return;
    }
    if (s.inBattle || !GameThread::IsGameThread() || !HostCheck::GroupOk("flow")) return;
    bool slotChanged = s.day != lastDay || s.time != lastTime;
    if (!slotChanged && s.frame - lastFrame < 600) return;
    lastFrame = s.frame;
    auto r = Flow::Call(Flow::Fn::GET_WEATHER, {});
    if (!r.ok) return;
    int8_t w = int8_t(int32_t(r.value));
    if (w != s.weather || slotChanged)
        LOGI("weather: %d on day %u, time slot %u%s", w, s.day, s.time, (w == 1 || w == 7) ? " (rain)" : "");
    s.weather = w;
    lastDay = s.day;
    lastTime = s.time;
}

}  // namespace

const char* ModeName(Mode m) {
    switch (m) {
        case Mode::Title: return "Title screen";
        case Mode::Field: return "Field";
        case Mode::Dungeon: return "Dungeon";
        case Mode::Battle: return "Battle";
        case Mode::Event: return "Event";
        case Mode::Menu: return "Menu";
        default: return "Unknown";
    }
}

void Capture() {
    Snapshot& s = g_snap;
    s.frame = ++g_frame;
    if (!HostCheck::GroupOk("core")) {
        s.valid = false;
        return;
    }
    const uintptr_t B = Addr::SaveBlock;

    s.day = Read<uint16_t>(B + Off::Date);
    s.time = Read<uint8_t>(B + Off::Date + 2);
    s.yen = Read<uint32_t>(B + Off::Yen);
    Copy(s.party, B + Off::Party);
    s.equippedSlot = Read<int16_t>(B + Off::EquippedSlot);
    Copy(s.units, B + Off::Units);
    Copy(s.stock, B + Off::Stock);
    Copy(s.links, B + Off::Links);
    Copy(s.items, B + Off::Items);
    Copy(s.flags, B + Off::Flags);
    Copy(s.counters, B + Off::Counters);

    if (uintptr_t f = Deref(Addr::pFieldIds)) {
        s.fieldMajor = Read<int32_t>(f);
        s.fieldMinor = Read<int32_t>(f + 4);
    }
    s.inBattle = Read<uint8_t>(Addr::InBattle) != 0 || Deref(Addr::pBattleMgr) != 0;
    s.campMenu = Deref(Addr::pCampMenu) != 0;

    // A save is loaded once the protagonist has a level and the calendar has started.
    s.valid = s.U(Yu).level > 0 && s.time <= 6;

    if (!s.valid) s.mode = Mode::Title;
    else if (s.inBattle) s.mode = Mode::Battle;
    else if (s.campMenu) s.mode = Mode::Menu;
    else s.mode = Mode::Field;  // dungeon / event split arrives with the context RE task

    CaptureWeather(s);
}

const Snapshot& Current() { return g_snap; }

uint16_t LinkThreshold(uint16_t commuId, uint16_t rank) {
    if (commuId >= 35 || rank > 10) return 0;
    uintptr_t t = Deref(Addr::pCommuTable);
    if (!t) return 0;
    return Read<uint16_t>(t + commuId * 100 + 0x18 + rank * 2);
}

uint8_t LinkArcana(uint16_t commuId) {
    if (commuId >= 35) return 0;
    uintptr_t t = Deref(Addr::pCommuTable);
    return t ? Read<uint8_t>(t + commuId * 100 + 0x10) : 0;
}

uint32_t ExpForLevel(int level) {
    if (level < 1 || level > 99 || !HostCheck::GroupOk("levels")) return 0;
    return Read<uint32_t>(Addr::McExpCurve + (level - 1) * 4);
}

uint8_t PersonaArcana(uint16_t id) {
    uintptr_t t = Deref(Addr::pPersonaTable);
    return (t && id < 256) ? Read<uint8_t>(t + id * 0xE + 2) : 0;
}

uint8_t PersonaBaseLevel(uint16_t id) {
    uintptr_t t = Deref(Addr::pPersonaTable);
    return (t && id < 256) ? Read<uint8_t>(t + id * 0xE + 3) : 0;
}

}  // namespace Game
