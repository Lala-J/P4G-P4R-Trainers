#pragma once
// Calls the game's own flowscript functions (the ones event scripts use, like INCREASE_SL or
// BIT_ON) with arguments we supply, so a change happens exactly the way a script would make it.
//
// How: a script function reads its arguments through GetFlowInput(i), which looks at the
// "current script context" *(0x1451FF778): sp at +0x20, argument type bytes at +0x23+slot,
// values at +0x50+slot*8 (slot = sp - i), and writes its result at +0x1D0 (type at +0x53).
// FlowCaller fills a private context that way, points the global at it for the duration of the
// call, and restores it. Game thread only (scripts run there, between frames).

#include <cstdint>
#include <initializer_list>

namespace Flow {

struct Result {
    bool ok = false;      // the function exists and was called
    bool done = false;    // it returned 1 (finished); 0 means "call me again next frame"
    int64_t value = 0;    // its return value, if it has one
};

// id = group << 12 | index; groups: 0 common, 1 field, 3 event.
Result Call(uint16_t id, std::initializer_list<int64_t> args = {});

// Script functions that take more than a frame (shops, the Velvet Room, save screens) return 0 until
// they finish; a script keeps calling them every frame with the same context. CallYielding does
// the same: it re-calls the function on each following frame, with its context preserved, until
// it reports done (or `maxFrames` pass). `onDone` runs on the game thread afterwards.
void CallYielding(uint16_t id, std::initializer_list<int64_t> args, int maxFrames = 60 * 60 * 30,
                  void (*onDone)(bool finished) = nullptr);
bool YieldingBusy();

// A short script: each step runs (yielding as needed) before the next starts, exactly like the
// game's own field scripts. Only one sequence runs at a time.
struct Step {
    uint16_t id;
    int64_t a[4];
    int argc;
};
bool RunSequence(const Step* steps, int count, const char* what);

// Expected argument count for a function id (-1 if unknown).
int ArgCount(uint16_t id);

// Function ids used by the trainer (Atlus-Script-Tools P4G library names).
namespace Fn {
constexpr uint16_t BIT_CHK = 0x000A;
constexpr uint16_t BIT_ON = 0x000B;
constexpr uint16_t BIT_OFF = 0x000C;
constexpr uint16_t GET_CNT = 0x0021;
constexpr uint16_t SET_CNT = 0x0022;
constexpr uint16_t GET_SOCIAL_STATS = 0x0042;
constexpr uint16_t VELVET_ROOM = 0x004B;
constexpr uint16_t DAIDARA_SHOP = 0x0049;
constexpr uint16_t SHIROKU_SHOP = 0x004A;
constexpr uint16_t ADD_YEN = 0x0050;
constexpr uint16_t CROCOFUR_SHOP = 0x007F;
constexpr uint16_t BOOK_SHOP = 0x0080;
constexpr uint16_t PRODUCE_SHOP = 0x0081;
constexpr uint16_t LEARN_SL_SKILL = 0x0084;
constexpr uint16_t ACCEPT_QUEST = 0x3039;
constexpr uint16_t COMPLETE_QUEST = 0x303A;
constexpr uint16_t START_SL = 0x3016;
constexpr uint16_t SAVE_ASK = 0x0092;
constexpr uint16_t FADE = 0x0016;
constexpr uint16_t FADE_SYNC = 0x0017;
constexpr uint16_t CALL_FIELD = 0x0030;
constexpr uint16_t NS_SAVE = 0x0057;
constexpr uint16_t SHIROKU_PUB = 0x007E;
constexpr uint16_t FLD_UNLOAD = 0x1024;  // FLD_FUNCTION_0024: field scripts call it before VELVET_ROOM
constexpr uint16_t GET_FIELD_MAJOR = 0x1022;
constexpr uint16_t GET_FIELD_MINOR = 0x1023;
constexpr uint16_t GOTO_REL_FLOOR = 0x1001;
constexpr uint16_t GET_FLOOR_ID = 0x1002;  // library name EVT_FUNCTION_0016: spawns the "commu open" card task
constexpr uint16_t GET_SL_LEVEL = 0x3003;
constexpr uint16_t INCREASE_SL = 0x3004;
constexpr uint16_t CHECK_IF_SL_LVLUP = 0x300E;
}  // namespace Fn

}  // namespace Flow
