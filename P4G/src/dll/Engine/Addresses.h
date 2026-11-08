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
constexpr uintptr_t pDungeonTask = 0x15E438348;  // dungeon floor task (GET_FLOOR_ID reads it) [DRV]
constexpr uintptr_t pCampMenu = 0x140EC0A40;    // non-null while the camp menu is open [MOD:AquaRose7]

// --- Natives -------------------------------------------------------------------------------
constexpr uintptr_t SetItemCount = 0x14019EC90;  // void (u16 id, u8 n), clamps 99  [DRV]
constexpr uintptr_t GetMaxHP = 0x1400CF510;      // thunk into .arch               [DRV]
constexpr uintptr_t GetMaxSP = 0x1400CF790;      // thunk; paired with GetMaxHP in the game's full heal [DRV]
constexpr uintptr_t AddHP = 0x1400CFA20;         // void (Unit*, int delta): hp = clamp(hp+delta, 0, max) [DRV]
constexpr uintptr_t AddSP = 0x1400CFA70;         // void (Unit*, int delta): sp = clamp(sp+delta, 0, max) [DRV]
constexpr uintptr_t CmmCanRankUp = 0x1401AF130;  // u16 (u16 id)                   [DRV]

// --- Battle rules (docs/data/battle.md) --------------------------------------------------
constexpr uintptr_t HitResolver = 0x1400D4650;        // [DRV] accuracy / evasion
constexpr uintptr_t CritResolver = 0x1400D4E40;       // [DRV][CT:qSleeP crit site inside]
constexpr uintptr_t GetAffinityResult = 0x1400D35B0;  // [MOD:AquaRose7][DRV]
constexpr uintptr_t GetAffinityKnown = 0x1400993F0;   // [MOD:AquaRose7][DRV]
constexpr uintptr_t Rand = 0x140520960;               // u32 () game RNG [DRV]
constexpr uintptr_t AiRunAway = 0x14006F7B0;          // AI 0x203F [DRV AI flow table 0x140AA9640]
constexpr uintptr_t AiWait = 0x14006F820;             // AI 0x2040
constexpr uintptr_t CanChangeToState = 0x140099180;   // [MOD:p4g64.quickBattleRestart sig][DRV]
constexpr uintptr_t PersonaCount = 0x140096650;       // u16 (1, 0): Yu's Persona count [DRV]
// --- Levels (battle-results sequence) ------------------------------------------------------
constexpr uintptr_t GenerateLevelUpPersona = 0x1400D7A90;  // (persona, u8 changes[0x88], gained) [MOD:p4g64.expShare][DRV]
constexpr uintptr_t ApplyLevelUp = 0x1400D7F20;            // thunk (persona, changes) [DRV GivePartyMembersExp]
constexpr uintptr_t PersonaReqExp = 0x1400D8050;           // thunk u32 (persona, level) [DRV]
constexpr uintptr_t GetPartyMemberPersona = 0x1400D6E80;   // thunk Persona* (charId) [DRV]
constexpr uintptr_t CalculateGainedExp = 0x140100F50;      // [MOD:p4g64.expShare]
constexpr uintptr_t GivePartyMembersExp = 0x140101AD0;     // [MOD:p4g64.expShare][DRV]
// --- Dialogue ---------------------------------------------------------------------------------
constexpr uintptr_t DrawDialog = 0x140461880;    // MsgWindow::DrawDialog(info)  [MOD:p4g64.accessibility][DRV]
constexpr uintptr_t CriSetCueId = 0x140611E4C;   // criAtomExPlayer_SetCueId     [MOD:Ryo][DRV]
constexpr uintptr_t CriGetStatus = 0x140610668;  // criAtomExPlayer_GetStatus    [MOD:Ryo][DRV]
// --- Persona stock & compendium (docs/data/personas.md) -------------------------------------
constexpr uintptr_t CreatePersona = 0x1400D8230;     // thunk void (Persona* out, u16 id): base form [DRV]
constexpr uintptr_t AddPersonaCopy = 0x1400D7450;    // int (const Persona*): free slot, copy, register; 0 = full [DRV]
constexpr uintptr_t RemoveStockSlot = 0x1400D71A0;   // int (u16 slot): shift down, re-find equipped [DRV]
constexpr uintptr_t FindStockById = 0x1400D6F30;     // thunk i16 (u16 id): slot or -1 [DRV]
constexpr uintptr_t StockCapacity = 0x1400D7630;     // int (): 6 / 8 / 10 / 12 [DRV]
constexpr uintptr_t CompendiumRegister = 0x140242DF0;  // void (const Persona*) [DRV]
constexpr uintptr_t SummonPrice = 0x140245630;       // int (const Persona*): the Velvet Room's fee [DRV]
constexpr uintptr_t pCompendium = 0x1411A7038;       // -> SaveBlock+0x2510, Persona[256] by id [DRV]
constexpr uintptr_t PersonaUnlocked = 0x140240420;   // int (u16 id): ultimates need their unlock flag [DRV]
constexpr uintptr_t pUnlockTable = 0x1411A7858;      // -> {i32 flag; i16 persona; u16 mode}[count] [DRV]
constexpr uintptr_t UnlockCount = 0x1411A7860;       // u32 [DRV]
// --- Dungeon minimap (docs/data/minimap.md) -------------------------------------------------
constexpr uintptr_t MinimapGrid = 0x1411AB39C;     // cell[24][16] x 16 B, index major*16+minor; +0: 1 walkable, 2 wall [MOD:AquaRose7][DRV]
constexpr uintptr_t pMinimapSeen = 0x1451EC580;    // -> u16[24]: bit `minor` of [major] = drawn on the minimap [DRV]
constexpr uintptr_t MinimapRevealCell = 0x14042FA80;  // void (u8 minor, u8 major): the game's own per-cell reveal [DRV]
constexpr uintptr_t pFieldRoot = 0x140EC0FF0;      // ->+0x250->+0x48->+0x18 = player; +0x360 x, +0x368 z [DRV]
constexpr uintptr_t pEnemyTable = 0x140EC0920;        // -> 0x3C per enemy id [MOD:AquaRose7][DRV]
constexpr uintptr_t pEnemyAffinity = 0x140EC0970;     // -> u16[16] per enemy id: hi byte flags [DRV]
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
     "40 53 48 83 EC 20 8B D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 8B 42 20 2B C3 4C 63 C0 41 0F BE 4C 10 23", 0},
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
    {"GetMaxSP thunk", "battle", ProbeKind::Thunk, GetMaxSP, nullptr, 0},
    {"Hit resolver", "battlefx", ProbeKind::Code, HitResolver,
     "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 20 66 44 89 40 18 57 41 54 41 55 41 56 41 57", 0},
    {"Crit resolver", "battlefx", ProbeKind::Code, CritResolver,
     "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 40 48 8B F1", 0},
    {"Crit roll", "battlefx", ProbeKind::Code, 0x1400D50E6, "45 3B C6 7C 0D 0F BA E7 1C", 0},
    {"GetAffinityResult", "battlefx", ProbeKind::Code, GetAffinityResult,
     "48 89 5C 24 18 55 57 41 54 41 56 41 57 48 83 EC 20 48 8B F9 44 0F BF F2", 0},
    {"GetAffinityKnown", "battlefx", ProbeKind::Code, GetAffinityKnown,
     "48 89 5C 24 08 57 48 83 EC 20 0F B7 D9 0F B7 FA", 0},
    {"rand", "battlefx", ProbeKind::Code, Rand, "48 83 EC 28 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 0D", 0},
    {"AI_RUN_AWAY", "battlefx", ProbeKind::Code, AiRunAway,
     "40 53 48 83 EC 20 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 33 C9 48 8B 98 58 02 00 00 E8 ?? ?? ?? ?? 25 FF FF FF 00 4C 8D 0D ?? ?? ?? ?? 0F BA E8 1A", 0},
    {"AI_WAIT", "battlefx", ProbeKind::Code, AiWait,
     "40 53 48 83 EC 20 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 33 C9 48 8B 98 58 02 00 00 E8 ?? ?? ?? ?? 25 FF FF FF 00 4C 8D 0D ?? ?? ?? ?? 0D 00 00 00 05", 0},
    {"GenerateLevelUpPersona", "levels", ProbeKind::Code, GenerateLevelUpPersona,
     "48 89 5C 24 20 48 89 54 24 10 48 89 4C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 30", 0},
    {"ApplyLevelUp thunk", "levels", ProbeKind::Thunk, ApplyLevelUp, nullptr, 0},
    {"PersonaReqExp thunk", "levels", ProbeKind::Thunk, PersonaReqExp, nullptr, 0},
    {"GetPartyMemberPersona thunk", "levels", ProbeKind::Thunk, GetPartyMemberPersona, nullptr, 0},
    {"CalculateGainedExp", "levels", ProbeKind::Code, CalculateGainedExp,
     "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 8B F1 48 8B FA", 0},
    {"GivePartyMembersExp", "levels", ProbeKind::Code, GivePartyMembersExp,
     "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC 20 48 8B D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 4C 8D B3 9A 06 00 00", 0},
    {"MsgWindow::DrawDialog", "dialogue", ProbeKind::Code, DrawDialog,
     "48 89 5C 24 10 48 89 6C 24 18 57 48 83 EC 20 48 8B D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B7 05", 0},
    {"criAtomExPlayer_SetCueId", "dialogue", ProbeKind::Code, CriSetCueId,
     "48 8B C4 48 89 58 ?? 48 89 68 ?? 48 89 70 ?? 48 89 78 ?? 41 54 41 56 41 57 48 81 EC 80 00 00 00 4D 63 F0", 0},
    {"criAtomExPlayer_GetStatus", "dialogue", ProbeKind::Code, CriGetStatus,
     "48 89 5C 24 ?? 57 48 83 EC 20 48 8B F9 48 85 C9 75 ?? 44 8D 41 ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8D 47", 0},
    {"Enemy affinity table use", "intel", ProbeKind::Code, 0x1400D363E,
     "48 8B 05 2B D3 DE 00 0F B7 14 48", 0},
    {"CanChangeToState", "battlefx", ProbeKind::Code, CanChangeToState,
     "48 89 5C 24 10 57 48 83 EC 20 48 8B F9 0F B7 DA 48 8D 0D", 0},
    {"CreatePersona thunk", "stock", ProbeKind::Thunk, CreatePersona, nullptr, 0},
    {"FindStockById thunk", "stock", ProbeKind::Thunk, FindStockById, nullptr, 0},
    {"AddPersonaCopy", "stock", ProbeKind::Code, AddPersonaCopy,
     "40 57 48 83 EC 20 48 8B F9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 66 83 F8 FF 75 08", 0},
    {"RemoveStockSlot", "stock", ProbeKind::Code, RemoveStockSlot,
     "48 89 5C 24 10 57 48 83 EC 20 0F B7 D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 66 85 DB 78 05 E8", 0},
    {"StockCapacity", "stock", ProbeKind::Code, StockCapacity,
     "48 83 EC 28 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 8B 88 40 02 00 00 F6 C1 04", 0},
    {"CompendiumRegister", "stock", ProbeKind::Code, CompendiumRegister,
     "40 53 48 83 EC 20 48 8B D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B7 43 02 48 8D 14 40 48 C1 E2 04 48 03 15", 0},
    {"SummonPrice", "stock", ProbeKind::Code, SummonPrice,
     "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 48 8B D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F BE 43 21", 0},
    {"Compendium pointer", "stock", ProbeKind::PtrEq, pCompendium, nullptr, SaveBlock + 0x2510},
    {"PersonaUnlocked", "fusion", ProbeKind::Code, PersonaUnlocked,
     "48 89 5C 24 08 57 48 83 EC 20 0F B7 D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 44 8B 15 ?? ?? ?? ?? 45 33 C9 BF 01 00 00 00", 0},
    {"Minimap reveal-cell", "minimap", ProbeKind::Code, MinimapRevealCell,
     "48 89 5C 24 10 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 30 0F B6 D9 48 8D 0D ?? ?? ?? ?? 0F B6 FA", 0},
    {"Minimap seen-bits use", "minimap", ProbeKind::Code, 0x14042FB28, "4C 8B 0D 51 CA DB 04 48 8D 4B F0", 0},
    {"Minimap grid use", "minimap", ProbeKind::Code, 0x1403105FD, "48 8D 0D 98 AD E9 00 41 B8 00 18 00 00", 0},
    {"Player position use", "minimap", ProbeKind::Code, 0x140430A03,
     "48 8B 05 E6 05 A9 00 0F 57 FF 45 0F 57 C0 48 8B 88 50 02 00 00 48 8B 41 48 48 8B 48 18", 0},
    {"AddHP", "battle", ProbeKind::Code, AddHP,
     "48 89 5C 24 08 57 48 83 EC 20 48 8B F9 8B DA 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B7 57 08", 0},
    {"AddSP", "battle", ProbeKind::Code, AddSP,
     "48 89 5C 24 08 57 48 83 EC 20 48 8B F9 8B DA 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B7 57 0A", 0},
};

}  // namespace Addr
