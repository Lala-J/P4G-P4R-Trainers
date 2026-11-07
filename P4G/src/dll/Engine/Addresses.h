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

// Pointer cells into the save block, used by the game's own code
constexpr uintptr_t pDate = 0x141165820;        // -> SaveBlock+0x18DC  [MOD:p4g64.riseOutfit][DRV]
constexpr uintptr_t pLinkArray = 0x1411A6AC8;   // -> SaveBlock+0x18E8 (array at +8) [DRV cmmCanRankUp]
constexpr uintptr_t pPartyInfo = 0x1411A5948;   // -> SaveBlock+0xC74   [MOD:p4g64.riseOutfit][DRV]

// --- Static data tables (in the game's own language) ----------------------------------------
constexpr uintptr_t pItemNames = 0x141165938;   // -> char[0x18] x 2560 [MOD:AquaRose7][DRV]
constexpr uintptr_t ItemNames = 0x141190160;
constexpr uintptr_t ArcanaNames = 0x141042120;  // char[0x15] x 32, id 0 = "00"   [DRV]
constexpr uintptr_t SkillNames = 0x1410423C0;   // char[0x17] x 624                [DRV]
constexpr uintptr_t EnemyNames = 0x141045BD0;   // char[0x15] x 368, id 0 = "000"  [DRV]
constexpr uintptr_t PersonaNames = 0x141047A00; // char[0x15] x 256, id 0 = "000"  [DRV]
constexpr uintptr_t pPersonaTable = 0x140EC0958;  // -> {u16 flags; u8 arcana; u8 lvl; u8 st[5]; ..} x 0xE [MOD:AquaRose7][DRV]
constexpr uintptr_t PersonaTable = 0x14104DFA0;
constexpr uintptr_t pCommuTable = 0x1411A6B10;  // -> heap, 100 B per link: +0x10 arcana, +0x18 u16 threshold[rank] [DRV cmmCanRankUp]
constexpr uintptr_t McExpCurve = 0x14090DBF0;   // u32[100]: total EXP to reach level L = [L-1] [MOD:AquaRose7][DRV]
constexpr uintptr_t LinkRankGate = 0x141B07220;  // u32; rank-ups blocked while non-zero [DRV]

// --- Context signals ---------------------------------------------------------------------
constexpr uintptr_t pFieldIds = 0x140AA8098;    // -> {i32 major, i32 minor, ...}  [MOD:MultiPresence][DRV]
constexpr uintptr_t FieldIds = 0x140ECA340;
constexpr uintptr_t InBattle = 0x140ECA358;     // u8                              [MOD:MultiPresence]
constexpr uintptr_t pBattleMgr = 0x140EC08F0;   // non-null during battle          [MOD:AquaRose7]
constexpr uintptr_t pCampMenu = 0x140EC0A40;    // non-null while the camp menu is open [MOD:AquaRose7]

// --- Natives -------------------------------------------------------------------------------
constexpr uintptr_t SetItemCount = 0x14019EC90;  // void (u16 id, u8 n), clamps 99  [DRV]
constexpr uintptr_t GetMaxHP = 0x1400CF510;      // thunk into .arch               [DRV]
constexpr uintptr_t CmmCanRankUp = 0x1401AF130;  // u16 (u16 id)                   [DRV]
constexpr uintptr_t CmmGetRank = 0x1401AE7F0;    // u16 (u16 id)                   [DRV]

enum class ProbeKind : uint8_t {
    Code,      // bytes at addr match sig
    Thunk,     // E9 rel32 at addr, target inside .arch
    PtrEq,     // *(u64*)addr == value
    TableFn,   // *(u64*)addr points into .shared
    LazyPtrEq, // like PtrEq, but a still-null pointer passes (filled in after boot)
    LazyCode,  // like Code, but still-zero memory passes (table loaded after boot)
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
    {"Date pointer", "core", ProbeKind::PtrEq, pDate, nullptr, SaveBlock + 0x18DC},
    {"Link array pointer", "links", ProbeKind::PtrEq, pLinkArray, nullptr, SaveBlock + 0x18E8},
    {"Party pointer", "core", ProbeKind::PtrEq, pPartyInfo, nullptr, SaveBlock + 0xC74},
    {"Item names pointer", "names", ProbeKind::LazyPtrEq, pItemNames, nullptr, ItemNames},
    {"Persona table pointer", "names", ProbeKind::LazyPtrEq, pPersonaTable, nullptr, PersonaTable},
    {"Field ids pointer", "core", ProbeKind::PtrEq, pFieldIds, nullptr, FieldIds},
    // Name tables: placeholder entries are language-independent ("00", "000", "100").
    {"Arcana names", "names", ProbeKind::LazyCode, ArcanaNames, "30 30 00", 0},
    {"Persona names", "names", ProbeKind::LazyCode, PersonaNames, "30 30 30 00", 0},
    {"Enemy names", "names", ProbeKind::LazyCode, EnemyNames, "30 30 30 00", 0},
    {"Skill names", "names", ProbeKind::LazyCode, SkillNames + 0x100 * 0x17, "31 30 30 00", 0},
    {"EXP curve", "levels", ProbeKind::Code, McExpCurve, "00 00 00 00 1C 00 00 00 42 00 00 00", 0},
    {"cmmCanRankUp", "links", ProbeKind::Code, CmmCanRankUp, "48 89 5C 24 10 48 89 6C 24 18 56 48 83 EC 20 0F B7 F1", 0},
};

}  // namespace Addr
