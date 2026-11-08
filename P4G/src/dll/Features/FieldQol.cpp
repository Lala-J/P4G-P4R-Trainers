#include "Features/FieldQol.h"

#include <string>

#include "Engine/FlowCaller.h"
#include "Engine/GameState.h"
#include "Engine/Addresses.h"
#include "Engine/GameThread.h"
#include "Engine/SigScan.h"
#include "Features/Journal.h"
#include "Util/Log.h"

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

int CurrentFloorId() {
    if (!GameThread::IsGameThread()) return -1;
    auto r = Flow::Call(Flow::Fn::GET_FLOOR_ID, {});
    return r.ok ? int(r.value) : -1;
}

void GotoRelativeFloor(int delta) {
    GameThread::Post([delta] {
        if (!CanRunNow(nullptr)) return;
        if (!InDungeon()) return;
        Flow::Step seq[1] = {{Flow::Fn::GOTO_REL_FLOOR, {delta}, 1}};
        if (!Flow::RunSequence(seq, 1, "change floor")) return;
        Journal::Record(delta > 0 ? "Moved to the next floor" : "Moved to the previous floor", nullptr);
    });
}

}  // namespace FieldQol
