# Memory layout, native functions and existing 64-bit mods

> Research notes compiled while planning the trainer (November 2026), from public mods,
> cheat tables, wikis and read-only inspection of the Steam build (P4G.exe v1.0.5,
> PE timestamp 0x64074B8B). "Your exe" / "your install" means that build. Anything not
> marked as verified must be checked against the game before the trainer relies on it;
> verified facts end up in `src/dll/Engine/Addresses.h` with a provenance tag.

I found a lot, and much of it is confirmed against your own `P4G.exe` (436,027,328 bytes on disk). I checked it read-only: I compared bytes, read the section table and the function tables, and disassembled a few functions with capstone. Nothing was written.

The main finding: your exe is the **v1.0.5 64-bit build**, and all the game's persistent state sits in **one global block at the fixed address `0x1451BCD70` (`P4G.exe+0x51BCD70`)**. The 2020 32-bit cheat table's offsets within that block still work unchanged on 64-bit.

## 0. Build identity
- Atlus labels the 1.04 update (Jan 19 2023) as the 32→64-bit switch, and 1.05 came out Apr 20 2023. A PE timestamp of 2023-03-07 fits v1.0.5.
  - Two pieces of evidence: AquaRose7's hard-coded bytes appear at the exact same file offset in your exe (`41 0F 10 45 00 49 8B CE F2 41 0F 11 46 50` at file 0x2D3668 = VA 0x1402D4068), and addresses posted in 2024–2025 resolve correctly against the block.
  - A May 2023 forum post gives a 1.04-era address (`P4G.exe+4A18B56`) that no longer works, so the layout moved between 1.04 and 1.05.
- **ASLR is off.** DllCharacteristics = `0x8120`, with no DYNAMIC_BASE flag, and the image base is always 0x140000000. Static addresses below are therefore constant from run to run.
- Section layout of your exe:
  - `.shared`: virtual address 0x140001000, file offset 0x600. This is the game code; file offset = VA − 0x140000A00.
  - `.sdata`: 0x140887000.
  - `.xpdata`: 0x140A20000, virtual size 0x1DA1A000 (data and BSS; the global block lives here).
  - `.arch`: 0x15E500000, size 0x19334DB6. This holds virtualized code (see section 6).

## 1. In-memory global block (64-bit v1.0.5)
Sources:
- zarroboogs 32-bit CT (relative offsets, originally the Vita base `0x814B301C`).
- Two 64-bit FearLessRevolution (FRF) tables from 2025–2026.
- AquaRose7 mod (verified live).
- MultiPresence (Discord presence tool).

```
BLOCK = 0x1451BCD70  (= P4G.exe+0x51BCD70; pointer cell *(0x141165900) also -> BLOCK)
+0x0000 u32      Yen
+0x0004 u16[3]   active party char ids
+0x000C          surname (SJIS) ; +0x001E given name (SJIS)
+0x0030 u8[2560] inventory count by item id. Id bases:
                 0x000 weapon, 0x100 armor, 0x200 accessory, 0x300 consumable,
                 0x400 key/event, 0x500 material, 0x600 skill card, 0x700 outfit,
                 0x800 consumable (Golden), 0x900 weapon (Golden). Max id 2559.
+0x0A30 s16      equipped MC stock slot (0..11)
+0x0A34 Persona[12] MC persona stock (0x30 each)
+0x0C74 Unit[8]  party records, stride 0x84  -> 0x1451BD9E4; index = charId-1
                 (1 MC, 2 Yosuke, 3 Chie, 4 Yukiko, 5 Rise, 6 Kanji, 7 Naoto, 8 Teddie)
+0x119C u8[]     event flags: bit n = byte[n>>3] bit (n&7)
+0x14DC u32[256] counters (zarroboogs calls these "Globals"):
                 [38] Reaper-chest counter, [56] cafe visits today, [124] play time,
                 [127] difficulty, [215] has caught bugs today
+0x18DC Date:    +0 u16 day index (0 = Apr 1; a new game starts at 9 = Apr 10)
                 +2 u8 time of day (0 Early Morning, 1 Morning, 2 Lunch, 3 Afternoon,
                    4 After School, 5 Evening, 6 Night)
                 +4 u32 (NEXT_DATE writes 1 here)
                 +8 u16 next day ; +0xA u8 next time
+0x18F0 SLink[23] stride 0x10: +0 u16 link/commu id, +2 u16 rank, +4 u16 points,
                 +0xC u8 flag (fendevel save tool; meaning unclear)
+0x1A60 book progress   +0x1B28 item info / shop availability
+0x2510 Persona[] compendium, indexed by persona id (entry = +0x2510 + id*0x30)
+0x5510 skill-card flags   +0x55F8 Enepedia
+0x9710 fusion counters, 4 bits per persona id: byte id/2, nibble (id%2)*4
+0x9790 trophy counters: +0 u32 fusions, +4 All-Out Attacks, +5 weakness exploits,
        +6 Rise lines heard, +F8 sweeps, +F9 Tanaka purchases, +FA perfect lunches,
        +FB movies, +FC SLinks maxed, +FD Nanako visits
+0x9890 Hollow Forest stash (HF yen at +0xA58)    +0xA30C name block

Persona (0x30): +0 u8 registered/valid, +2 u16 id, +4 u8 level, +8 u32 exp,
  +0xC u16 skills[8], +0x1C u8 St,Ma,En,Ag,Lu, +0x21 bonus stats[5], +0x26 other stats[5]
  (displayed stat = sum of the three arrays, capped at 99)
Unit (0x84): +0 u16 flags (bit0 = in party, 0x200 = guard?), +2/+4 ids,
  +6 u8 MC level, +8 u16 HP, +0xA u16 SP, +0xC u32 ailments,
  +0x14..+0x2C buff flags / direction / duration nibbles,
  +0x34 u16 Courage, +0x36 Knowledge, +0x38 Diligence, +0x3A Understanding, +0x3C Expression,
  +0x40 u32 MC total EXP, +0x4C/+0x4E/+0x50/+0x52 weapon/armor/accessory/outfit item ids,
  +0x54 embedded Persona for party members
```

Independent checks that line up with this layout:
- FRF post: Hardcore Risette counter at `P4G.exe+51C6506` = BLOCK+0x9796. Reaper chests at `P4G.exe+51BE2E4` = +0x14DC + 38*4.
- qSleeP table: romance "flag 2383" at `1451BE035` = +0x119C + 2383/8.
- AquaRose7: wallet at 0x1451BCD70 and party array at 0x1451BD9E4 = +0xC74.
- expShare: `PartyInfo{+4 InParty[3], +0xA30 ActivePersonaSlot, +0xA34 ProtagPersonas[12]}`.

**Social stat order:** AquaRose7 (live-verified) and zarroboogs both put Courage first, at +0x34. NickDriscoll's labels (Knowledge first) are wrong.

Other useful facts:
- Known flags (zarroboogs, riseOutfit):
  - 48–54 party members unlocked.
  - 2048 NG+ / custom difficulty menu, 2049 custom difficulty menu in system-menu-only mode.
  - 2383–2390 and 2621 romance.
  - 3121 field collision.
  - 3752 / 3760 Striptease bonus boss item / boss available.
  - 5187 Golden ending.
- Social stat rank thresholds (AquaRose7):
  - Knowledge 30/80/150/240
  - Courage 16/40/80/140
  - Diligence 16/40/80/130
  - Understanding 16/40/80/140
  - Expression 13/33/53/85

## 2. Pointer globals and signatures, resolved against your exe
Static, from AquaRose7: camp menu `*(0x140EC0A40)`, battle `0x140EC08F0`, task lists `0x1462486F8/0x1462486A8/0x146248768`, MC name `0x141165908`, item-name table pointer `0x141165938`, MC EXP curve `int[100] @0x14090DBF0`, commu table `*(0x1411A6B10)` (stride 100, +0x10 = arcana byte), item data `*(0x1411A5940)` (stride 0x44), persona affinity `*(0x140EC0988)`, species/arcana `*(0x140EC0958)` (stride 0xE).

From MultiPresence: in-battle byte `0x140ECA358`. Field major/minor are ints at +0/+4 of `*(0x140AA8098)`, whose initial value in the file is 0x140ECA340.

Signatures as published, followed by what each one resolves to in your exe:
```
expShare     "48 8B 15 ?? ?? ?? ?? 66 89 87 ?? ?? ?? ??"  (+3)  -> 0x141165900 (-> BLOCK)
accessibility"48 8B 15 ?? ?? ?? ?? 41 0F 28 FA"           (+3)  -> 0x141165900
riseOutfit FlagsPtr  "48 8B 0D ?? ?? ?? ?? F6 81 ?? ?? ?? ?? 40"         (+3) -> 0x1451FF7A0 (-> flag array)
riseOutfit DatePtr   "4C 8B 15 ?? ?? ?? ?? 8B 4C 24 ??"                  (+3) -> 0x141165820 (-> BLOCK+0x18DC)
riseOutfit PartyInfo "48 8B 0D ?? ?? ?? ?? 4C 8D 67 ??"                  (+3) -> 0x1411A5948 (-> 0x1451BD9E4)
riseOutfit ItemsPtr  "48 8B 1D ?? ?? ?? ?? 45 33 C0 80 BB ?? ?? ?? ?? 01" (+3) -> 0x141165930 (-> BLOCK+0x30)
quickBattleRestart "48 8D 0D ?? ?? ?? ?? 48 8B D3 E8 ?? ?? ?? ?? E9 ?? ?? ?? ?? 81 FF 40 03 00 00" (+3, lea) -> 0x1451BD9E4
SegTimeDayWrite "48 8B 05 ?? ?? ?? ?? 66 89 58 08" (+3) -> 0x141165820
SegMiscPtr      "48 8B 05 ?? ?? ?? ?? 8B 08 83 F9 17 75" (+3) -> 0x140AA8098   [2 hits]
p4g64.lib Language "8B 05 ?? ?? ?? ?? 85 C0 74 ?? 33 D2" (+2) -> 0x140FD01E4
debugStuff Tasks "4C 8B 0D ?? ?? ?? ?? 33 C9 4C 8B 15 ?? ?? ?? ?? 44 8B C1 4C 8B 1D ?? ?? ?? ?? 41 8B D0 45 85 C0 74 ?? 83 EA 01 74 ??" + "83 FA 01 75 ?? 49 8B CB EB ?? 49 8B CA EB ?? 49 8B C9 48 85 C9 74 ?? 0F 1F 80 00 00 00 00" (+3/+12/+22) -> 0x1462486F8...
   TaskInfo: +0x1C LockInputs (bit 0x10), +0x48 Args, +0x50 NextTask
```
Two published signatures don't match your exe, or match ambiguously:
- expShare `GetPartyMemberPersona`, `GetTotalRequiredExp` and `HiddenLevelUpPartyMember` have 0 hits. They are written with `48 89 CF` / `48 89 CE`, but MSVC emits `48 8B F9`. Re-derive these before relying on them.
- DeathChaos25 levelcap "Get Current Day Ptr" has 0 hits.

## 3. Flowscript function tables (found in your exe)
I matched argument counts from AtlusScriptCompiler's P4G library against your exe. The Field table matches AquaRose7's note ("Field COMM dispatch table at 0x140AA80D0"). Each entry is `{u64 fn; u32 argc; u32 pad}`:
- Field (0x1000+): `0x140AA80D0`, 80 entries.
- Common (0x0000+): `0x140AA85D0`, 192 entries.
- Event (0x3000+): `0x140AA91D0`, 78 entries.
- Next table at `0x140AA96B0`: probably AI (0x2000+), unverified.

How the native functions work:
- Each is `nuint fn(void)`. Arguments come from `GetFlowInput(int idx)` at `0x14046AC20`, which reads the current script context `*(0x1451FF778)`.
- Results go to ctx+0x1D0, with ctx+0x53 set to 0 for an int result.
- persistentBgm's `"40 53 48 83 EC 20 8B D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 8B 42 ??"` hits exactly 0x14046AC20. Its `Flowscript::CALL_BATTLE` signature `"41 56 41 57 48 83 EC 48 48 8D 0D ?? ?? ?? ??"` hits 0x140321F20.

```
0x000A BIT_CHK 0x140466C30 | 0x000B BIT_ON 0x140466C90 | 0x000C BIT_OFF 0x140466CF0
   (all three are single `jmp` thunks into .arch; ADD_YEN 0x0050 @0x140426F70 too)
0x0033 GET_MONTH 0x140425E30  0x003D NEXT_DATE 0x1404261F0  0x0042 GET_SOCIAL_STATS 0x140426B20
0x0052 GET_ITEM 0x140427040   0x0053 SET_ITEM 0x140427090  0x005F GET_TOT_HP 0x140427C70
0x0060 SET_HP 0x140427CC0     0x0063 SET_SP 0x140427DB0    0x0038/39 ADD/RM_PARTY 0x1404270E0/0x140427210
Event: 0x3003 GET_SL_LEVEL 0x1401C34B0, 0x3004 INCREASE_SL 0x1401C3540,
       0x3011 REVERSE_SL 0x1401C3840, 0x3012 BREAK_SL 0x1401C3980
```
Plain functions you can call directly (I named these from the disassembly):
- `SetItemCount(u16 id, u8 n)` at `0x14019EC90`: clamps to 99 and writes `*(0x141165930)[id]`.
- `MonthDayToDayIndex(m, d)` at `0x14013BC80`: used by NEXT_DATE.
- `GetMaxHP(Unit*)` at `0x1400CF510`.
- `0x140426A40(0, stat, amount)`: what GET_SOCIAL_STATS calls. It queues a UI task, so it is probably "add social stat points with animation".

## 4. Sources

**64-bit Reloaded-II mods (raw URLs; signatures beyond section 2):**
- **AnimatedSwine37/p4g64.expShare**: [Native.cs](https://raw.githubusercontent.com/AnimatedSwine37/p4g64.expShare/HEAD/p4g64.expShare/Native.cs), [Mod.cs](https://raw.githubusercontent.com/AnimatedSwine37/p4g64.expShare/HEAD/p4g64.expShare/Mod.cs). Signatures that hit your exe:
  - `SetupResultsExp "48 89 54 24 ?? 48 89 4C 24 ?? 53 55 56 57 41 54 41 56"`
  - `CalculateGainedExp "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 8B F1 48 8B FA"`
  - `GenerateLevelUpPersona "48 89 5C 24 ?? 48 89 54 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 30"`
  - `IsPartyMemberAvailable "40 53 48 83 EC 20 0F BF D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 33 D2"`
  - `GivePartyMembersExp` and `PartyMemberLevelUps` also hit.
  - BattleResults layout: `+0x74 PartyInfo{ +0x69A PartyMembers[4], +0x6A4 ExpGains[4] }`.
- **p4g64.debugStuff**: [Mod.cs](https://raw.githubusercontent.com/AnimatedSwine37/p4g64.debugStuff/HEAD/p4g64.debugStuff/Mod.cs), [Tasks.cs](https://raw.githubusercontent.com/AnimatedSwine37/p4g64.debugStuff/HEAD/p4g64.debugStuff/Native/Tasks.cs). It wraps the built-in debug viewers (`RunSprView "40 53 48 83 EC 40 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? BA 70 19 01 00"`, plus ScriptViewer, FontInfo, CpuGraph, HbnEditor and others) and hooks DebugPrint (`"E8 ?? ?? ?? ?? EB ?? BB 3D 00 00 00"`, which resolves to 0x1404A33F0). It hard-codes `_runDebugMenu = 0x1403db350` and, commented out, `_runCommunityEdit = 0x1401d62a0`.
- **p4g64.riseOutfit**: [Common.cs](https://raw.githubusercontent.com/AnimatedSwine37/p4g64.riseOutfit/HEAD/p4g64.riseOutfit/Native/Common.cs). Its `CheckFlag` shifts by `flag & 0x1f`, which is a bug; use `flag & 7`.
- **p4g64.persistentBgm**: [Mod.cs](https://raw.githubusercontent.com/AnimatedSwine37/p4g64.persistentBgm/HEAD/p4g64.persistentBgm/Mod.cs). **quickBattleRestart**: [Mod.cs](https://raw.githubusercontent.com/AnimatedSwine37/p4g64.quickBattleRestart/HEAD/p4g64.quickBattleRestart/Mod.cs), which gives `BtlInfo+0xCB8 → Turn; Turn+0x38 → unit; unit+0xCF0 → UnitInfo (HP +8)`.
- **p4g64.customSubMenu "Mod Menu"** is pure flowscript, no native code: [PlayerMenu.flow](https://raw.githubusercontent.com/AnimatedSwine37/p4g64.customSubMenu/HEAD/p4g64.customSubMenu/FEmulator/BF/ModMenu/PlayerMenu.flow). It uses ADD_YEN, SET_ITEM, NEXT_DATE, GET_SOCIAL_STATS, INCREASE_SL / REVERSE_SL / BREAK_SL and SET_HP / SET_SP.
- **p4g64.configFramework** draws with the game's own UI functions, not ImGui (`Spr::Draw "48 8B C4 55 53 57 48 8D A8 ?? ?? ?? ?? 48 81 EC 40 03 00 00"`), and reads input via `InputsPtr "F7 05 ?? ?? ?? ?? 00 40 00 00 74 ?? 44 8B EB"`.
- **p4g64.lib** only exposes the Language pointer.
- **AquaRose7/Persona-4-Golden-Access** (2026, the richest RE source): [Party.cs](https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/HEAD/p4g64.accessibility-master/p4g64.accessibility/Native/Party.cs), [PlayerMenu.cs](https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/HEAD/p4g64.accessibility-master/p4g64.accessibility/Components/CommandMenus/PlayerMenu.cs), [FieldTracker.cs](https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/HEAD/p4g64.accessibility-master/p4g64.accessibility/Components/FieldTracker.cs), [OVERWORLD.md](https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/HEAD/database/OVERWORLD.md), [BATTLE_SYSTEM.md](https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/HEAD/database/BATTLE_SYSTEM.md), [VELVET_ROOM.md](https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/HEAD/database/VELVET_ROOM.md) (fusion and compendium UI objects), [SOCIAL_LINK_READERS.md](https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/HEAD/database/SOCIAL_LINK_READERS.md). Signatures that hit your exe:
  - Shop: `"40 55 53 41 54 41 55 41 57 48 8B EC 48 83 EC 70 48 8B D9"` at 0x1402827F0.
  - Calendar: `"40 55 57 48 8D 6C 24 A8 48 81 EC 58 01 00 00 48 8D 0D C8 A9 35 1E 48 8B FA"`.
  - GetSkillCost: `"48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F1 0F B7 DA 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ??"`.
- **Other 64-bit mods:**
  - [seyleonhart/p4g64.InputLibrary64](https://raw.githubusercontent.com/seyleonhart/p4g64.InputLibrary64/HEAD/p4g64.InputLibrary64/Inputs.cs): keyboard `"85 DB 74 05 E8 ?? ?? ?? ?? 48 63 15 ?? ?? ?? ?? 4C 8D 0D ?? ?? ?? ?? 48 C1 E2 06"`; DirectInput pad `"48 8B 4B 08 C6 43 24 01 48 85 C9 74 ?? 48 8B 01 4C 8D 83 C0 06 00 00 BA 50 00 00 00 48 89 7C 24 30 FF 50 48 8B F8"` (hook at +0x24, DIJOYSTATE at [rbx+0x6C0]). Both hit your exe.
  - [DeathChaos25 levelcap](https://raw.githubusercontent.com/DeathChaos25/p4g64.patch.levelcap/HEAD/Mod.cs) has EXP distribution hooks.

**32-bit (2020) only, for reference:** AnimatedSwine37/p4gpc.tinyadditions, zarroboogs/p4gpc.tinyfixes, rirurin p4gpc.*, and ShrineFox/Persona-4-Golden-Mod-Menu (the flowscript predecessor of customSubMenu).
- [zarroboogs/p4g-ct](https://raw.githubusercontent.com/zarroboogs/p4g-ct/master/p4g-pc.CT) is the origin of the block layout. Its base AOB was `"?? ?? ?? ?? 8B 08 89 4D B0 85 F6 75 10"`.
- [NickDriscoll/persona4_cheats](https://raw.githubusercontent.com/NickDriscoll/persona4_cheats/main/src/main.rs) uses absolute 32-bit addresses (`TEAM_BASE_ADDR 0x04DDD034`, stride 0x84) that fit the same layout.

**Possible malware, not opened:** easyfrag46lucky/P4G-multiply-slink-point-mod, vpecnbrnkuqy/persona-4-golden-trainer, StreetComptroller11/Persona-4-Golden-Cheats, pointswanwave33/Persona-4-Golden-Save-Utility. They look like SEO or malware-lure repos.

**Cheat Engine tables:**
- **divergence, Sep 2025, 64-bit** ([download](https://fearlessrevolution.com/download/file.php?id=72752); thread [t=12819](https://fearlessrevolution.com/viewtopic.php?t=12819&start=255)): static base `P4G.exe+51BCD70`, Date +18DC, SLink +18F0/+18F2/+18F4, Compendium +2542, Status +CA8. It labels Teddie +0x294 and Naoto +0x318 relative to Yosuke, which looks swapped against the charId order.
  - Card-pick patch: `P4G.exe+12E300: dec [rsi+0001BA44]` (original bytes `FF 8E 44 BA 01 00`).
  - The same thread gives a save-checksum function at `P4G.exe+0x108110` (I did not verify this).
- **qSleeP v11, 2026, 64-bit** ([download](https://fearlessrevolution.com/download/file.php?id=81565); thread [t=39689](https://fearlessrevolution.com/viewtopic.php?t=39689)). All of these hit your exe:
  - `aobscanmodule(expHook,P4G.exe,85 FF 7E 23 B8 9F 86 01 00 44 3B C0 7F 1C 45 85 C0 41 8B C0 B9 01 00 00 00 0F 4E C1)`
  - `aobscanmodule(critYu,P4G.exe,45 3B C6 7C 0D)` (forced crit jumps to `P4G.exe+D50F8`)
  - `sflAlways 44 3B C0 73 2F 48 8B 1D`, which NOPs the `jae` at +3.
  - Rhark's trainer (v1.0.5, Jan 2025; [t=23602](https://fearlessrevolution.com/viewtopic.php?t=23602)) only lists features.
- RebelMods v1.0 ([t=28900](https://fearlessrevolution.com/viewtopic.php?t=28900)): its AOB `01 D1 88 1C 02` has 0 hits in your exe. OpenCheatTables blocked me with a Cloudflare challenge.

## 5. Save files
Sources: [fendevel/ShrineFox P4G-Save-Tool](https://raw.githubusercontent.com/fendevel/P4G-Save-Tool/HEAD/P4G%20Save%20Tool/P4G%20Save%20Tool/MainWindow.xaml.cs), [hcoona's rewrite](https://raw.githubusercontent.com/hcoona/P4G-Save-Tool/HEAD/src/P4G.SaveTool.SaveFormat/P4GSaveLayout.cs), [p4g-saveconv](https://raw.githubusercontent.com/zarroboogs/p4g-saveconv/master/convert_vita2pc.py), and [cfxxldev's checksum updater](https://raw.githubusercontent.com/cfxxldev/P4GPC_Savegame_Helper/master/P4GPC_Savegame_Checksum_Updater/P4GPC_Savegame_Checksum_Updater.cpp).

The save is a list of segments, each with an 8-byte header (`id u32, size u32`), and the data mirrors the memory block. Rule: save offset = memory offset + 0x58 + 8 × (number of later segments). These offsets are for Vita and the 2020 PC build:
```
0x10/0x22 names | 0x58 yen | 0x5C party | 0x88 inventory[2559] | 0xA8C MC stock 12x0x30
0xCDA MC level | 0xD08 social stats 5xu16 | 0xD14 MC EXP | 0xD20 MC equip | 0xDA4+i*0x84 party equip/persona
0x1204 flags (0x1304 = flag byte 0x100 -> flags 2048/2049, the custom-difficulty bits)
0x154C counters | 0x1954 date | 0x1970 SLinks | 0x25D8 compendium (persona id 1)
0x15120 rescue-request segment (PC size 0x3FA4) | 0x190D4 fusion counters | 0x1915C trophy counters
```
- The `0x19162` Risette-counter offset reported for 64-bit saves matches this rule exactly. So the 64-bit layout matches at least up to about 0x19170.
- 2020 PC checksums:
  - Byte-sum mod 256 of bytes 0x38..end, stored in a footer at 0x34D30 as `{0x2000, 1, sum, 0xFFFFFFFF}`.
  - `.binslot`: +0x08 holds MD5 of slot data (bytes 0x28..EOF + `"P4GOLDEN"`); +0x18 holds MD5 of the `.bin`.
- Your 217,937-byte saves are 0x614 bytes longer than the 2020 size, so the footer position on 64-bit is unknown.

## 6. Rendering, input, protection
- **No public P4G64 ImGui or D3D11 overlay mod exists** as far as I could find.
  - The usual Reloaded route is [Reloaded.Imgui.Hook](https://github.com/Sewer56/Reloaded.Imgui.Hook) with its `.Direct3D11` package, which hooks DXGI Present/ResizeBuffers. It's used by other games' mods, not P4G ones.
  - AnimatedSwine37 draws in-game UI with the game's Spr/Text functions instead.
- **Blocking game input while your overlay has focus:** hook the InputLibrary64 sites above (keyboard mask, DInput GetDeviceState at vtable +0x48), or set `TaskInfo.LockInputs |= 0x10` the way debugStuff does.
- **Steam overlay and Denuvo:** I found nothing specific to P4G64. Reloaded-II issue #78 reports Special K + Reloaded crashes, but that was the 32-bit era.
- **Protection:**
  - AquaRose7 says "Arxan hides xrefs" and mentions "VMProtect-thunk" cases. Frida crashed the game for them.
  - BIT_* , ADD_YEN and the persona-EXP function are `jmp` thunks into `.arch`. Call them, or hook their callers, but don't inline-hook the `.arch` targets.
  - Dozens of mods inline-hook `.shared` functions without trouble.
  - AquaRose7 also warns about two crashes: AsmHooks using `mov [abs],reg`, which gets encoded RIP-relative and the displacement truncates, and a hook in a hot loop at 0x1402EE068.

## Still uncertain
- Whether the u32[256] counter array is what GET_COUNT/SET_COUNT read. The library lists those at AI 0x20AD/0x20AE, but entry 0xAD of the 0x140AA96B0 table doesn't fit.
- Whether the 23-entry SLink count and the meaning of SLink +0xC hold.
- Whether zarroboogs' misc block at BLOCK−0x10 still exists on 64-bit. Use `0x140ECA340` for the field ids instead.
- The 64-bit save footer and checksum.
- Whether `P4G.exe+0x108110` is the checksum function.
- The helper function names in section 3, which are my inference.
