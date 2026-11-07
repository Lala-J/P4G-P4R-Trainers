#pragma once
// The audited address table for P4G.exe v1.0.5 (Steam, x64, PE timestamp 0x64074B8B).
//
// ASLR is off for this executable (DllCharacteristics 0x8120), so these are absolute.
// Real code lives in `.shared`; `.arch` holds the protector's virtualised thunks. A `.shared`
// function that starts with `E9 rel32` into `.arch` is a thunk: it may be CALLED, never hooked.
//
// Every entry records where it came from:
//   [DRV]          derived here, by disassembling the live process
//   [MOD:<repo>]   from a public mod's source, re-verified against this build
//   [CT:<table>]   from a Cheat Engine table, re-verified against this build
// and HostCheck verifies its probe (expected bytes) before any feature may use it. A failed
// probe disables only the features that depend on it.

#include <cstddef>
#include <cstdint>

namespace Addr {

constexpr uint32_t kTimestamp = 0x64074B8B;
constexpr uintptr_t kImageBase = 0x140000000;
constexpr uintptr_t kSharedStart = 0x140001000;  // .shared  [DRV]
constexpr size_t kSharedSize = 0x885600;
constexpr uintptr_t kArchStart = 0x15E500000;  // .arch     [DRV]
constexpr size_t kArchSize = 0x19334DB6;

// --- Save block ("the game state") ------------------------------------------------------
constexpr uintptr_t pSaveBlock = 0x141165900;  // -> SaveBlock   [CT:zarroboogs/p4g-ct][DRV]
constexpr uintptr_t SaveBlock = 0x1451BCD70;   // [DRV]

// --- Flowscript ----------------------------------------------------------------------------
constexpr uintptr_t FlowTableCommon = 0x140AA85D0;  // {fn,argc,pad} x192     [DRV]
constexpr uintptr_t FlowTableField = 0x140AA80D0;   // x80                    [DRV]
constexpr uintptr_t FlowTableEvent = 0x140AA91D0;   // x78                    [DRV]
constexpr uintptr_t GetFlowInput = 0x14046AC20;     // int (int idx)          [DRV]
constexpr uintptr_t pFlowContext = 0x1451FF778;     // -> current script ctx  [DRV]

// --- Natives -------------------------------------------------------------------------------
constexpr uintptr_t SetItemCount = 0x14019EC90;  // void (u16 id, u8 n), clamps 99  [DRV]
constexpr uintptr_t GetMaxHP = 0x1400CF510;      // thunk into .arch               [DRV]

enum class ProbeKind : uint8_t {
    Code,      // bytes at addr match sig
    Thunk,     // E9 rel32 at addr, target inside .arch
    PtrEq,     // *(u64*)addr == value
    TableFn,   // *(u64*)addr points into .shared
};

struct Probe {
    const char* name;
    const char* group;  // feature group that needs it ("core", "flow", "inventory", ...)
    ProbeKind kind;
    uintptr_t addr;
    const char* sig;    // Code: signature; others: unused
    uintptr_t value;    // PtrEq: expected value
};

inline constexpr Probe kProbes[] = {
    {"SaveBlock pointer", "core", ProbeKind::PtrEq, pSaveBlock, nullptr, SaveBlock},
    {"GetFlowInput", "flow", ProbeKind::Code, GetFlowInput,
     "40 53 48 83 EC 20 8B D9 48 8D 0D ?? ?? ?? ?? E8", 0},
    {"Flow table: Common[0]", "flow", ProbeKind::TableFn, FlowTableCommon, nullptr, 0},
    {"Flow table: Field[0]", "flow", ProbeKind::TableFn, FlowTableField, nullptr, 0},
    {"Flow table: Event[0]", "flow", ProbeKind::TableFn, FlowTableEvent, nullptr, 0},
    {"SetItemCount", "inventory", ProbeKind::Code, SetItemCount,
     "48 89 5C 24 08 57 48 83 EC 20 0F B7 F9 0F B6 DA", 0},
    {"GetMaxHP thunk", "battle", ProbeKind::Thunk, GetMaxHP, nullptr, 0},
};

}  // namespace Addr
