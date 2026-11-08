#include "Features/FieldQol.h"

#include <atomic>
#include <iterator>
#include <string>

#include "Engine/FlowCaller.h"
#include "Engine/GameState.h"
#include "Engine/Addresses.h"
#include "Engine/GameThread.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"
#include "Features/Journal.h"
#include "Util/Log.h"
#include "Util/Settings.h"

namespace FieldQol {

namespace {
uint16_t FnFor(Facility f) {
    switch (f) {
        case Facility::Velvet: return Flow::Fn::VELVET_ROOM;
        case Facility::Daidara: return Flow::Fn::DAIDARA_SHOP;
        case Facility::Shiroku: return Flow::Fn::SHIROKU_SHOP;
        case Facility::Crocofur: return Flow::Fn::CROCOFUR_SHOP;
        case Facility::Books: return Flow::Fn::BOOK_SHOP;
        case Facility::Produce: return Flow::Fn::PRODUCE_SHOP;
    }
    return 0;
}
}  // namespace

const char* FacilityName(Facility f) {
    switch (f) {
        case Facility::Velvet: return "Velvet Room";
        case Facility::Daidara: return "Daidara Metalworks";
        case Facility::Shiroku: return "Shiroku Store";
        case Facility::Crocofur: return "Crocofur (Junes)";
        case Facility::Books: return "Book store";
        case Facility::Produce: return "Junes produce";
    }
    return "?";
}

bool CanRunNow(const char** why) {
    const auto& s = Game::Current();
    if (Flow::YieldingBusy()) {
        if (why) *why = "Waiting for the last shortcut to finish.";
        return false;
    }
    if (!s.valid) {
        if (why) *why = "No save is loaded.";
        return false;
    }
    if (s.inBattle) {
        if (why) *why = "Not during a battle.";
        return false;
    }
    if (s.campMenu) {
        if (why) *why = "Close the game's menu first.";
        return false;
    }
    return true;
}

void OpenFacility(Facility f) {
    GameThread::Post([f] {
        const char* why = nullptr;
        if (!CanRunNow(&why) || InDungeon()) return;
        // The town's own sequence (field scripts f008 / f011): fade out, the facility, then
        // rebuild the current field. The player reappears at the area's main entrance.
        int64_t maj = Flow::Call(Flow::Fn::GET_FIELD_MAJOR, {}).value;
        int64_t min = Flow::Call(Flow::Fn::GET_FIELD_MINOR, {}).value;
        Flow::Step seq[5];
        int n = 0;
        seq[n++] = {Flow::Fn::FADE, {1, 10}, 2};
        seq[n++] = {Flow::Fn::FADE_SYNC, {}, 0};
        if (f == Facility::Velvet) seq[n++] = {Flow::Fn::FLD_UNLOAD, {}, 0};
        if (f == Facility::Books) seq[n++] = {FnFor(f), {0}, 1};
        else seq[n++] = {FnFor(f), {}, 0};
        seq[n++] = {Flow::Fn::CALL_FIELD, {maj, min, 0, 0}, 4};
        if (Flow::RunSequence(seq, n, FacilityName(f)))
            Journal::Record(std::string("Opened ") + FacilityName(f) + " (back in the same area afterwards)", nullptr);
    });
}

void SaveAnywhere() {
    GameThread::Post([] {
        if (!CanRunNow(nullptr)) return;
        Flow::Step seq[1] = {{Flow::Fn::NS_SAVE, {}, 0}};  // what save points call
        Flow::RunSequence(seq, 1, "save");
    });
}

bool InDungeon() {
    // A dungeon floor task exists only inside the TV world.
    if (!Sig::Readable(Addr::pDungeonTask, 8)) return false;
    return *reinterpret_cast<const uintptr_t*>(Addr::pDungeonTask) != 0;
}

namespace {
std::atomic<int> g_floorCache{-1};
}

int CurrentFloorId() {
    if (!GameThread::IsGameThread()) return g_floorCache.load();
    auto r = Flow::Call(Flow::Fn::GET_FLOOR_ID, {});
    int v = r.ok ? int(r.value) : -1;
    g_floorCache = v;
    return v;
}

int CurrentFloorIdCached() { return g_floorCache.load(); }

// Replays the game's own stairs, step for step (field/script/dungeon.bf; docs/data/stairs.md):
//   goto_stair    common_floor_change, get_reach_level, stairs sound, BIT_OFF 3167, fade, change
//   backto_stair  common_floor_change,                  stairs sound, BIT_OFF 3167, fade, change
//   common_floor_change  RESET_FLOOR; if counter 38 == 13: counter 38 = 14, flags 3118/3119 off
//   get_reach_level      record the floor reached in counters 9/10 (35/36 in floors 160..178)
// Skipping common_floor_change's RESET_FLOOR left the old floor's state behind, and the game
// crashed shortly after arriving (a field job looked up a room for a cell that has none).
namespace {
constexpr int kStairsFlag = 1024 + 2048 + 95;  // 3167
constexpr int kUpViaCall[] = {6, 12, 30, 50, 69, 88, 109, 121, 148, 169};    // goto_stair
constexpr int kDownViaCall[] = {6, 8, 21, 41, 61, 81, 101, 121, 141, 161};   // backto_stair

bool In(const int* a, size_t n, int v) {
    for (size_t i = 0; i < n; ++i)
        if (a[i] == v) return true;
    return false;
}

int64_t Cnt(int i) {
    auto r = Flow::Call(Flow::Fn::GET_CNT, {i});
    return r.ok ? r.value : -1;
}

bool Bit(int b) {
    auto r = Flow::Call(Flow::Fn::BIT_CHK, {b});
    return r.ok && r.value;
}

// The stairs sound for this dungeon (goto_stair / backto_stair pick it by floor range).
// Returns the system sound id, 0 when the game plays sound effect 705 instead.
int StairsSound(int floor) {
    bool stairsBit = Bit(kStairsFlag);
    auto in = [floor](int lo, int hi) { return floor >= lo && floor < hi; };
    if (in(20, 40)) return 27;
    if ((!stairsBit && in(100, 120)) || in(40, 60)) return 25;
    if (in(80, 100)) return 26;
    if (!stairsBit && in(120, 140)) return 0;
    return 24;
}
}  // namespace

const char* WhyNotChangeFloor(int delta) {
    const char* why = nullptr;
    if (!CanRunNow(&why)) return why;
    if (!InDungeon()) return "Only inside a dungeon.";
    if (delta < 0 && CurrentFloorIdCached() == 123) return "Use the stairs on this floor.";
    return nullptr;
}

void GotoRelativeFloor(int delta) {
    if (delta != 1 && delta != -1) return;
    GameThread::Post([delta] {
        if (WhyNotChangeFloor(delta)) return;
        int floor = CurrentFloorId();
        if (floor < 0) return;
        if (delta < 0 && floor == 123) return;
        Flow::Step seq[16];
        int n = 0;
        // common_floor_change
        seq[n++] = {Flow::Fn::RESET_FLOOR, {}, 0};
        if (Cnt(38) == 13) {
            seq[n++] = {Flow::Fn::SET_CNT, {38, 14}, 2};
            seq[n++] = {Flow::Fn::BIT_OFF, {1024 + 2048 + 46}, 1};
            seq[n++] = {Flow::Fn::BIT_OFF, {1024 + 2048 + 47}, 1};
        }
        // get_reach_level (going up only)
        if (delta > 0) {
            int a = 9, b = 10;
            if (floor >= 160 && floor < 179) a = 35, b = 36;
            int64_t ca = Cnt(a), cb = Cnt(b);
            if (ca >= 0 && floor >= ca) seq[n++] = {Flow::Fn::SET_CNT, {a, floor + 1}, 2};
            if (cb >= 0 && floor >= cb) seq[n++] = {Flow::Fn::SET_CNT, {b, floor + 1}, 2};
        }
        int sound = StairsSound(floor);
        if (sound == 0) seq[n++] = {Flow::Fn::PLAY_SOUNDEFFECT, {705}, 1};
        seq[n++] = {Flow::Fn::BIT_OFF, {kStairsFlag}, 1};
        if (sound) seq[n++] = {Flow::Fn::PLAY_SYS_SOUNDEFFECT, {1, sound}, 2};
        seq[n++] = {Flow::Fn::FADE, {1, 10}, 2};
        seq[n++] = {Flow::Fn::FADE_SYNC, {}, 0};
        if (delta > 0) {
            if (In(kUpViaCall, std::size(kUpViaCall), floor)) seq[n++] = {Flow::Fn::CALL_DUNGEON, {floor + 1, 0}, 2};
            else if (floor == 122) seq[n++] = {Flow::Fn::CALL_DUNGEON, {129, 0}, 2};
            else seq[n++] = {Flow::Fn::GOTO_REL_FLOOR, {1}, 1};
        } else {
            if (In(kDownViaCall, std::size(kDownViaCall), floor)) seq[n++] = {Flow::Fn::CALL_DUNGEON, {floor - 1, 1}, 2};
            else seq[n++] = {Flow::Fn::GOTO_REL_FLOOR, {-1}, 1};
        }
        if (!Flow::RunSequence(seq, n, "change floor")) return;
        LOGI("floor change from %d (%+d)", floor, delta);
        Journal::Record(delta > 0 ? "Moved to the next floor" : "Moved to the previous floor", nullptr);
    });
}

// --- Minimap auto-reveal -------------------------------------------------------------------------
namespace {

constexpr int kMajor = 24, kMinor = 16, kCell = 16;
bool g_reveal = false;
bool g_loaded = false;
int g_frame = 0;
int g_shown = -1, g_total = 0;

using RevealCellFn = void(__fastcall*)(uint8_t minor, uint8_t major);

const uint8_t* Grid() { return reinterpret_cast<const uint8_t*>(Addr::MinimapGrid); }
uint8_t CellFlag(int major, int minor) { return Grid()[(major * kMinor + minor) * kCell]; }
// Open sides (low nibble S/E/N/W). Zero on the unused corners of L-shaped room footprints, which
// the game never draws.
uint8_t CellEdges(int major, int minor) { return Grid()[(major * kMinor + minor) * kCell + 0xA]; }

uint16_t* SeenBits() {
    if (!Sig::Readable(Addr::pMinimapSeen, 8)) return nullptr;
    auto p = *reinterpret_cast<uintptr_t*>(Addr::pMinimapSeen);
    if (!p || !Sig::Readable(p, kMajor * 2)) return nullptr;
    return reinterpret_cast<uint16_t*>(p);
}

uintptr_t Follow(uintptr_t a, size_t off) {
    if (!a || !Sig::Readable(a + off, 8)) return 0;
    return *reinterpret_cast<uintptr_t*>(a + off);
}

// The player's minimap cell, from the same chain and rounding the game's reveal pass uses.
bool PlayerCell(int* major, int* minor) {
    uintptr_t pl = Follow(Follow(Follow(Follow(Addr::pFieldRoot, 0), 0x250), 0x48), 0x18);
    if (!pl || !Sig::Readable(pl + 0x360, 12)) return false;
    float x = *reinterpret_cast<float*>(pl + 0x360);
    float z = *reinterpret_cast<float*>(pl + 0x368);
    if (!(x >= 0.f && z >= 0.f)) return false;  // also rejects NaN
    int mi = int((x + 600.f) / 1200.f), ma = int((z + 600.f) / 1200.f);
    if (mi < 0 || mi >= kMinor || ma < 0 || ma >= kMajor) return false;
    *major = ma;
    *minor = mi;
    return true;
}

// The grid and the seen bits belong to the floor the player is on: the player stands on a
// walkable cell of a room (non-zero room id) that the game itself has already revealed.
// (Not every floor has a wall ring around the grid, so the edges can't be used as a check.)
bool FloorLive(uint16_t* seen) {
    int ma, mi;
    if (!PlayerCell(&ma, &mi)) return false;
    if (CellFlag(ma, mi) != 1 || !((seen[ma] >> mi) & 1)) return false;
    uint16_t room = *reinterpret_cast<const uint16_t*>(Grid() + (ma * kMinor + mi) * kCell + 2);
    return room != 0;
}

void LoadOnce() {
    if (g_loaded) return;
    g_loaded = true;
    g_reveal = Settings::GetInt("field", "revealMinimap", 0) != 0;
}

}  // namespace

bool RevealMapAvailable() { return HostCheck::GroupOk("minimap"); }

bool RevealMapEnabled() {
    LoadOnce();
    return g_reveal;
}

void SetRevealMap(bool on) {
    LoadOnce();
    g_reveal = on;
    Settings::SetInt("field", "revealMinimap", on ? 1 : 0);
}

int RevealedCells(int* total) {
    if (total) *total = g_total;
    return g_shown;
}

void Tick() {
    LoadOnce();
    if (++g_frame % 10) return;  // a few times a second is plenty
    g_shown = -1;
    g_total = 0;
    if (!InDungeon()) {
        g_floorCache = -1;
        return;
    }
    if (Sig::Readable(Addr::InBattle, 1) && *reinterpret_cast<const uint8_t*>(Addr::InBattle)) return;
    if (g_frame % 60 == 0 && !Flow::YieldingBusy()) CurrentFloorId();  // keeps the UI's copy fresh
    if (!RevealMapAvailable()) return;
    uint16_t* seen = SeenBits();
    if (!seen || !FloorLive(seen)) return;
    auto reveal = reinterpret_cast<RevealCellFn>(Addr::MinimapRevealCell);
    int shown = 0, total = 0, done = 0;
    for (int ma = 1; ma < kMajor - 1; ++ma)
        for (int mi = 1; mi < kMinor - 1; ++mi) {
            if (CellFlag(ma, mi) != 1 || !CellEdges(ma, mi)) continue;
            ++total;
            if ((seen[ma] >> mi) & 1) {
                ++shown;
                continue;
            }
            if (!g_reveal) continue;
            reveal(uint8_t(mi), uint8_t(ma));
            ++done;
            if ((seen[ma] >> mi) & 1) ++shown;
        }
    g_shown = shown;
    g_total = total;
    if (done) LOGI("minimap: revealed %d cell(s), %d/%d shown", done, shown, total);
}

// --- Return to the entrance (dng_escape) ------------------------------------------------------
namespace {
struct DungeonRange {
    int lo, hi, counter, entrance;
};
// dungeon.bf dng_escape: per dungeon (by floor id), the "continue from" counter and the entrance.
constexpr DungeonRange kDungeons[] = {
    {5, 20, 23, 5},     {20, 40, 24, 20},   {40, 60, 25, 40},   {60, 80, 26, 60},  {80, 100, 27, 80},
    {100, 120, 28, 100}, {120, 140, 29, 120}, {140, 160, 30, 140}, {160, 180, 37, 160},
};
const DungeonRange* RangeOf(int floor) {
    for (const auto& r : kDungeons)
        if (floor >= r.lo && floor < r.hi) return &r;
    return nullptr;
}
}  // namespace

const char* WhyNotEscape() {
    const char* why = nullptr;
    if (!CanRunNow(&why)) return why;
    if (!InDungeon()) return "Only inside a dungeon.";
    int floor = CurrentFloorIdCached();
    const DungeonRange* r = RangeOf(floor);
    if (!r) return "Not on a dungeon floor.";
    if (floor == r->entrance) return "You're already at the entrance.";
    return nullptr;
}

void ReturnToEntrance() {
    GameThread::Post([] {
        if (WhyNotEscape()) return;
        int floor = CurrentFloorId();
        const DungeonRange* r = RangeOf(floor);
        if (!r || floor == r->entrance) return;
        // The game refuses here (its "can't escape" message) when flag 3165 is off and 3164 on.
        if (!Bit(1024 + 2048 + 93) && Bit(1024 + 2048 + 92)) {
            LOGI("return to entrance: the game doesn't allow leaving from here");
            return;
        }
        int reached = floor;
        auto s36 = Flow::Call(Flow::Fn::STAIRS_FOUND, {});
        auto s37 = Flow::Call(Flow::Fn::FLOOR_OBJ6_FOUND, {});
        if (s37.ok && s36.ok && s37.value == 0 && s36.value == 1) reached = floor + 1;
        Flow::Step seq[8];
        int n = 0;
        if (Cnt(38) == 13) {
            seq[n++] = {Flow::Fn::SET_CNT, {38, 14}, 2};
            seq[n++] = {Flow::Fn::BIT_OFF, {1024 + 2048 + 46}, 1};
            seq[n++] = {Flow::Fn::BIT_OFF, {1024 + 2048 + 47}, 1};
        }
        bool record = !(r->counter == 29 && (floor == 121 || floor == 122 || floor == 129));
        if (record) seq[n++] = {Flow::Fn::SET_CNT, {r->counter, r->counter == 37 ? floor : reached}, 2};
        // dng_escape plays TRAESTO_VISUAL here, but that function attaches its effect to the calling
        // script's owner (context +0x240), which a trainer call doesn't have (it crashed the game).
        // The stairs' fade stands in for the warp animation.
        seq[n++] = {Flow::Fn::FADE, {1, 10}, 2};
        seq[n++] = {Flow::Fn::FADE_SYNC, {}, 0};
        seq[n++] = {Flow::Fn::CALL_DUNGEON, {r->entrance, 1}, 2};
        if (!Flow::RunSequence(seq, n, "return to entrance")) return;
        LOGI("return to entrance from floor %d (continue from %d)", floor, reached);
        Journal::Record("Returned to the dungeon entrance (as a Goho-M)", nullptr);
    });
}

// --- Party ------------------------------------------------------------------------------------
namespace {
int JoinFlag(int charId) {
    static const int flag[9] = {-1, -1, 48, 49, 50, 52, 51, 54, 53};  // by character id
    return charId >= 2 && charId <= 8 ? flag[charId] : -1;
}
}  // namespace

bool PartyAvailable(int charId) {
    int f = JoinFlag(charId);
    return f >= 0 && Game::Current().Flag(f);
}

bool InParty(int charId) {
    for (uint16_t id : Game::Current().party)
        if (id == charId) return true;
    return false;
}

const char* WhyNotParty(int charId, bool add) {
    const char* why = nullptr;
    if (!CanRunNow(&why)) return why;
    if (!PartyAvailable(charId)) return "Hasn't joined yet.";
    if (add) {
        if (InParty(charId)) return "Already in the party.";
        int n = 0;
        for (uint16_t id : Game::Current().party) n += id != 0;
        if (n >= 3) return "The party is full (Yu and three others).";
    } else if (!InParty(charId)) {
        return "Not in the party.";
    }
    return nullptr;
}

void SetInParty(int charId, bool in) {
    GameThread::Post([charId, in] {
        Game::Capture();
        if (WhyNotParty(charId, in)) return;
        Flow::Call(in ? Flow::Fn::ADD_PARTY : Flow::Fn::RM_PARTY, {charId});
        static const char* n[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
        Journal::Record(std::string(n[charId]) + (in ? " joined the party" : " left the party"), [charId, in] {
            Flow::Call(in ? Flow::Fn::RM_PARTY : Flow::Fn::ADD_PARTY, {charId});
        });
    });
}

// --- The Fox --------------------------------------------------------------------------------
void FoxRecovery() {
    GameThread::Post([] {
        const char* why = nullptr;
        if (!CanRunNow(&why) || !InDungeon()) return;
        Flow::Step seq[12];
        int n = 0;
        seq[n++] = {Flow::Fn::FADE, {1, 5}, 2};
        seq[n++] = {Flow::Fn::SET_FADE_COLOUR, {255, 255, 255}, 3};
        seq[n++] = {Flow::Fn::FADE_SYNC, {}, 0};
        int members[4] = {1, 0, 0, 0};
        int m = 1;
        for (uint16_t id : Game::Current().party)
            if (id && m < 4) members[m++] = id;
        for (int i = 0; i < m; ++i) {
            auto tot = Flow::Call(Flow::Fn::GET_TOT_SP, {members[i]});
            if (tot.ok) seq[n++] = {Flow::Fn::SET_SP, {members[i], tot.value}, 2};
        }
        seq[n++] = {Flow::Fn::FADE_BACK, {15}, 1};
        if (Flow::RunSequence(seq, n, "fox recovery")) Journal::Record("The Fox restored the party's SP", nullptr);
    });
}

}  // namespace FieldQol
