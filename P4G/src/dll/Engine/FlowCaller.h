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
constexpr uint16_t START_SL = 0x3016;  // library name EVT_FUNCTION_0016: spawns the "commu open" card task
constexpr uint16_t GET_SL_LEVEL = 0x3003;
constexpr uint16_t INCREASE_SL = 0x3004;
constexpr uint16_t CHECK_IF_SL_LVLUP = 0x300E;
}  // namespace Fn

}  // namespace Flow
