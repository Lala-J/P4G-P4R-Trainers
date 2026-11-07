# Flowscript state: flags, counters, script functions

> Research notes compiled while planning the trainer (November 2026), from public mods,
> cheat tables, wikis and read-only inspection of the Steam build (P4G.exe v1.0.5,
> PE timestamp 0x64074B8B). "Your exe" / "your install" means that build. Anything not
> marked as verified must be checked against the game before the trainer relies on it;
> verified facts end up in `src/dll/Engine/Addresses.h` with a provenance tag.

I couldn't find any single source that documents P4G's script state end to end. What follows is pieced together from the Atlus Script Tools libraries, community flowscript mods, a 32-bit Cheat Engine table, a 64-bit accessibility mod, and a PS2 Persona 4 decompilation. Every item is tagged with how solid it is: **[P4G lib]** = Atlus-Script-Tools P4G library, **[mod code]** = used in working P4G mod scripts, **[32-bit]** = old PC build only, **[64-bit]** = current Steam build, **[PS2]** = vanilla P4 decompilation only, **[inferred]** = my own deduction.

## Raw URLs to re-fetch during implementation
```
# Atlus-Script-Tools P4G library (identical in the Persona-Modding fork; ShrineFox's P4G function page is empty)
https://raw.githubusercontent.com/tge-was-taken/Atlus-Script-Tools/master/Source/AtlusScriptLibrary/Libraries/Persona4Golden/FlowScriptModules.json
.../Persona4Golden/Modules/Common/Functions.json   (0x0000-0x00BF, 192 functions)
.../Persona4Golden/Modules/Common/Enums.json       (SocialStat, BattleStat, EquipmentType, Expression, FadeType, MessageVarType)
.../Persona4Golden/Modules/Event/Functions.json    (0x3000-0x304D, social-link functions)
.../Persona4Golden/Modules/Field/Functions.json    (0x1000-0x104F)
.../Persona4Golden/Modules/AI/Functions.json       (0x2000-0x2125)
.../Persona4Golden/Modules/AI/Enums.json           (PartyUnit, PersonaUnit 0-255, BattleSkill 0-575)
.../Persona4Golden/MessageScriptLibrary.json       (slAffinity, giveitem, flag)
.../Persona4/Modules/Event/Functions.json          (vanilla P4 names: CMM_EXIST, CMM_RANKUP, ...)
# Enum tables (zarroboogs Cheat Engine table, 32-bit PC)
https://raw.githubusercontent.com/zarroboogs/p4g-ct/master/data/{cmm,arcana,persona,item,skill,char}.tsv
https://raw.githubusercontent.com/zarroboogs/p4g-ct/master/p4g-pc.CT
https://raw.githubusercontent.com/AnimatedSwine37/p4gpc.tinyadditions/master/p4gpc.tinyadditions/Utilities/P4GEnums.cs
# Real P4G scripts that change state
https://raw.githubusercontent.com/ShrineFox/Persona-4-Golden-Mod-Menu/main/ModMenu/{PlayerMenu,FlagMenu,Utilities}.flow
https://raw.githubusercontent.com/ShrineFox/Persona-4-Golden-Mod-Menu/main/OtherMods/FindAFriend.flow
https://raw.githubusercontent.com/AnimatedSwine37/Find-a-Friend/master/Find%20A%20Friend%20Bits.txt
https://raw.githubusercontent.com/AnimatedSwine37/p4g64.customSubMenu/master/p4g64.customSubMenu/FEmulator/BF/ModMenu/PlayerMenu.flow
https://raw.githubusercontent.com/AnimatedSwine37/p4g64.customSubMenu/master/p4g64.customSubMenu/FEmulator/BF/OtherMods/ConsistentReaper.flow
https://raw.githubusercontent.com/AnimatedSwine37/p4g64.customSubMenu/master/p4g64.customSubMenu/FEmulator/BF/OtherMods/DungeonOptions/DungeonTravel.flow
# 64-bit addresses and structs
https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/main/database/{SOCIAL_LINK_READERS,VELVET_ROOM,TV_LISTINGS,OVERWORLD}.md
https://raw.githubusercontent.com/AquaRose7/Persona-4-Golden-Access/main/p4g64.accessibility-master/p4g64.accessibility/Native/{Party,PersonaInfo,Item}.cs
# PS2 P4 decompilation (how the social-link system works internally)
https://raw.githubusercontent.com/Raikaru/Persona4-Decompilation/main/docs/architecture.md
https://raw.githubusercontent.com/Raikaru/Persona4-Decompilation/main/src/{cmmCommunity,cmmScript,cmmRankUp}.c
```

---

## 1. Flowscript functions
Function IDs are `group<<12 | index`. The groups are 0 Common, 1 Field, 2 AI, 3 Event, 4 Facility, 5 Net. "P4:" gives the vanilla PS2 name where it differs.

**Flags and counters [P4G lib]**
```
0x000A BIT_CHK(int bitId) -> int        0x000B BIT_ON(int bitId)        0x000C BIT_OFF(int bitId)
0x0021 GET_CNT(int INDEX) -> int        0x0022 SET_CNT(int INDEX, int AMOUNT)
0x20AD GET_COUNT(int) / 0x20AE SET_COUNT(int,int)   -- AI module, battle-side counters (assumed separate)
```
The mod menu's memory editor works only because BIT_ON, BIT_OFF and BIT_CHK do **no bounds checking**: out-of-range IDs read and write arbitrary memory. Validate every flag ID before calling them.

**Items and money [P4G lib]**
```
0x0052 GET_ITEM(int index) -> int        "Returns amount of the item index you currently have"
0x0053 SET_ITEM(int index, int count)    -- there is no ADD_ITEM; use SET_ITEM(id, GET_ITEM(id)+n)
0x0050 ADD_YEN(int Yen_Amount) -> int    "Adds the amount of yen and return the new total." (P4: ADD_PC_MONEY)
       Mod menu sets an exact value with:  ADD_YEN(0 - ADD_YEN(0)); ADD_YEN(new_yen);
0x0054 GET_EQUIPMENT_ID(PartyUnit, int) -> int
0x0055 SET_EQUIPMENT(PartyUnit, EquipmentType{Weapon0,Armor1,Accessory2,Clothes3}, int equipmentId)
Message-script: [giveitem itemId itemAmount]   [item colorIndex ? itemId]
```

**Personas and skills [P4G lib]**
```
0x0045 INCREASE_PERSONA_STAT(PartyUnit, BattleStat{St0,Ma1,En2,Ag3,Lu4}, int amount)  (protagonist = equipped Persona)
0x0046 INCREASE_PERSONA_STAT2(...)       "appears to do exactly the same thing"
0x0047 GET_PERSONA_STAT(PartyUnit, BattleStat) -> int
0x0048 GET_CURRENT_PERSONA() -> int
0x005B FIRST_PERSONA_REBIRTH(PartyUnit)  -- party member Persona evolution
0x0084 LEARN_SL_SKILL(int,int) -> int    0x0099 FUNCTION_0099(int,int) -> int ("LEARN_BONUS_SKILL")
0x009E SCOOTER_SKILL_AVAILABLE(PartyUnit,int) -> int   0x009F LEARN_BONUS_SKILL_BEACH(int) -> int
0x004B VELVET_ROOM()                     0x008D CALL_MARIE_DNG_TV()
```
**Gap:** no P4G library documents a function to add, remove or register a Persona, or to fuse. Organic acquisition goes through the Velvet Room UI. Stock data is known (sections 3 and 4).

**Social links, Event module [P4G lib, with P4 names and PS2 behaviour]**
```
0x3002 EVT_FUNCTION_0002(int) -> int      P4: CMM_EXIST(cmmId) -> bool
0x3003 GET_SL_LEVEL(int) -> int           P4: CMM_GET_LV(cmmId)
0x3004 INCREASE_SL(int)                   PS2: cmmCommunityTryRankUp (rank+1 if points >= threshold, points=0)
0x3009 EVT_FUNCTION_0009(int,int)         PS2: cmmMsgStreamStart(kind, cmmId) - sets the "current" social link
0x300E CHECK_IF_SL_LVLUP(int) -> int      P4: CMM_CHK_LVUP(cmmId)
0x3011 REVERSE_SL(int,int)                P4: CMM_SET_REVERSE(cmmId, bool value)
0x3012 BREAK_SL(int,int)                  P4: CMM_SET_BROKEN(cmmId, bool value)
0x3014 EVT_FUNCTION_0014(int)             PS2: cmmScriptSelectEntry -> cmmCommunityAdvanceForm (romance row switch)
0x3016 EVT_FUNCTION_0016(int)             PS2: cmmScriptSetCurrentEntry (0 clears); used by mod menu "Add SL"
0x302C SL_LVLUP_VISUAL(int,int,int)       P4: CMM_RANKUP(cmmId, rank, param3) - presentation only
0x3031 EVT_FUNCTION_0031(int,int,int)     P4: CMM_ADD_POINT_ID(cmmId, param2, param3) -> scaled point add
0x3039 ACCEPT_QUEST(int) / 0x303A COMPLETE_QUEST(int)   P4: NPC_QUEST_START/END(questId)
0x303D EVT_FUNCTION_003D()                P4G only (the P4 table ends at 0x303C); mod menu calls it after SL_LVLUP_VISUAL
```
The other PS2 meanings (0x3019 top Persona of the link's arcana, 0x301D set marked, 0x3022 resolve counterpart) come from `cmmScript.c` and are **[PS2]**. Every index where both the P4 and P4G names exist agrees, which makes the PS2 semantics a good guess for P4G but not a guarantee.

**Social stats [P4G lib]**
```
0x0042 GET_SOCIAL_STATS(SocialStat, int increaseAmount) -> int   -- the name is misleading: it ADDS points (P4: SET_PC_PARAM)
0x0044 GET_SOCIAL_STAT_LEVEL(SocialStat) -> int (1-5)          (P4: GET_PC_PARAM)
SocialStat: Courage=0 Knowledge=1 Diligence=2 Understanding=3 Expression=4   (mod menu "max all" adds 240 to each)
```

**Date, time and weather [P4G lib]**
```
0x0033 GET_MONTH() 0x0034 GET_DAY_OF_MONTH() 0x0035 GET_DAY_OF_WEEK() (0-6) 0x0036 GET_TIME_OF_DAY()
       time: 0 EarlyMorning 1 Morning 2 Lunchtime 3 Afternoon 4 AfterSchool 5 Evening   (CT lstTime; Find-a-Friend treats 5 as night)
0x0037 DATE_CHK(int,int) -> int           P4: CHK_DAYS(month, day)
0x0040 CHECK_TIME_SPAN(date1Month, date1Day, date2Month, date2Day) -> int
0x003D NEXT_DATE(int,int,int)             mod menu: NEXT_DATE(month, day, 0); then FADE + TV_STUDIO() to force a transition
0x004C GET_WEATHER() -> int               "0=Clear,1= Rain, 2= Cloudy, Thunderstorm" (fog not documented)
0x0029 WEATHER_FORECAST(msg)  0x0066 SHOW_DATE(int)  0x007D OPEN_CALENDAR()  0x00A4 TRANSITION_EPILOGUE()
```
In the PS2 code, the scheduler runs procedure `sdl%02d_%02d_%s` (suffix AM_A, AM_B, PM_A, PM_B, PM_C or PM_D) from `scheduler/scheduler_%02d.bf`. P4G mod hooks use the same names, for example `sdl04_10_PM_D_hook`.

**Party [P4G lib]**
```
0x0038 ADD_PARTY(int) 0x0039 RM_PARTY(int)    (P4: PARTY_IN/PARTY_OUT; ids 2-8)
0x005A GET_PARTY_MEMBER_ID(int slot 0-3; 0 = protagonist)
0x005E GET_CUR_HP 0x005F GET_TOT_HP 0x0060 SET_HP(PartyUnit,int) 0x0061 GET_CUR_SP 0x0062 GET_TOT_SP 0x0063 SET_SP
```
Inside a dungeon, the mod menu reloads the floor with `CALL_DUNGEON(floor,0)` or `CALL_FLOOR(floor)` so a party change takes effect.

**Other useful [P4G lib]**
```
0x0019 CALL_PROCEDURE(int procedureIndex, int)   (P4: SCR_RUN)
0x0030 CALL_FIELD(major,minor,location,weather)  0x0032 CALL_EVENT(major,minor,sub)
0x0067 NG_PLUS_SAVE()  0x006A BAD_END_CREDITS()  0x0090 SAVE_STORY()
```

**Message-script (.msg) functions [P4G lib]**
```
[slAffinity socialLinkId increaseAmount modelId]   (group 5 fn 13; socialLinkId 0 = the link set by EVT_FUNCTION_0009(0, slId))
[slAffinityInvis socialLinkId increaseAmount]      (group 5 fn 8)
[flag 0 0 flagId state]                            (group 2 fn 7; "use at the end of a selection option")
```

## 2. Known flags and counts

**Flag layout [inferred, consistent across sources]**

Game scripts build flag IDs as section-base sums, for example `BIT_CHK(0 + 0x0400 + 0x0800 + 608)`.
```
S0 0x0000 (size 0x400)  "0 + n"
S1 0x0400 (0x800)       "0 + 0x0400 + n"
S2 0x0C00 (0x400)       "0 + 0x0400 + 0x0800 + n"
S3 0x1000 / S4 0x1200 / S5 0x1400 (0x200 each; not seen alone)
S6 0x1600 (0x400?)      "0 + 0x0400 + 0x0800 + 0x0400 + 0x0200 + 0x0200 + 0x0200 + n"
Total 0x1A00 bits = 0x340 bytes. Storage: byte = id/8, bit = id%8.
```
Three things back this up. The 32-bit Cheat Engine table puts the flag block exactly 0x340 bytes before the count block. The flag-pointer signature in tinyadditions ends with `81 C6 40 03 00 00` (add esi, 0x340). The S3–S5 sizes and the S6 size are inferred from that total.

**Social link flags**
- **Romance [32-bit CT, absolute IDs]:** `2383 Marie, 2384 Chie, 2385 Yukiko, 2386 Rise, 2388 Ai, 2389 Yumi, 2390 Ayane, 2621 Naoto`.
- **Jester to Hunger switch [CT]:** `BIT_OFF 1054`, `BIT_ON 1055`, `BIT_ON 2069`. 1054 = 0x3FF+31 and 1055 = 0x3FF+32, which matches the PS2 "link exists" formula below.
- **PS2 formula [PS2 only]:** `flag = cmmId + 0x3FF + 32*band`, where band 0 = exists, 1 = max, 2 = reversed, 3 = broken, 4–12 = ranks 1–9. In P4G, band 0 fits, but a stride of 32 can't hold IDs 33/34 (Marie), so P4G's other bands are unverified.
- **Availability flags from FindAFriend.flow [mod code]:** S1-relative offsets (add 0x400 for the absolute ID). What each flag means is not documented.
  - Two day sets, chosen by S1+1569:
    - Set A: Yosuke 432, Kanji 433, Chie 434, Yukiko 435, Rise 436.
    - Set B: Nanako 449, Rise 451, Yukiko 452, Yosuke 454, Dojima 455, Sayoko 456, Kanji 457, Chie 458, Naoto 461/462, Kou/Daisuke 463, Naoki 465, Hisano 466, Ai 468, Shu 470, Yumi/Ayane 475, Eri 476.
  - Others: Adachi 692/702, Marie 712 and 527, Shu 1638, Sayoko 1640, Eri 1641.
  - Scooter rides: 624–630 (Rise, Yukiko, Yosuke, Kanji, Chie, Naoto, Teddie).
  - Night availability: 1152–1163 (Yosuke, Chie, Yukiko, Kanji, Rise, Naoto, Kou, Daisuke, Yumi, Ayane, Naoki, Ai).
  - Night blocks: S2+883..896.
  - Busy after a Marie outing: S1+1216..1221 = absolute 2240–2245 (Yosuke, Chie, Yukiko, Kanji, Rise, Naoto).
  - Global blockers: absolute 87, and 2512 (Ai is unavailable on the day her link starts).
  - Fox rank gates: completed request flag and not S1+1696..1704.
  - Discrepancy: Find-a-Friend Bits.txt gives Chie as 1483, but the code uses S1+458 = 1482. Trust the code.

**Other flags**
- **Party members [CT, meaning unlabeled]:** 48–54 = Yosuke, Chie, Yukiko, Kanji, Rise, Teddie, Naoto. Dungeon scripts check 52 for Rise as navigator.
- **Quests [Amicitia]:** unlocked = 255+id, completed = 319+id, new = 383+id. There is a second range at 6400–6495.
- **Dungeon [mod code]:** Reaper active = S2+46 (3118), Reaper encounter = S2+47. The map-unlock set is S2+173..181 plus S6+85, applied together with `SET_CNT(62,3)` and `SET_CNT(63,3)`.
- **Avoid S6 numbers used by mods for their own settings:** 5678, 6324, 6328, 6331, 6334, 6335.
- **Not found anywhere:** flags for the true or bad ending path, NG+ or clear data, victims rescued, or dungeons cleared. One mod approximates "dungeon beaten" by date window instead: `CHECK_TIME_SPAN(m,d,3,30)` with Yukiko 4/30, Kanji 6/5, Rise 7/10, Mitsuo 8/13, Naoto 10/6, Nanako 11/21, Adachi 12/25.

**Counts (GET_CNT/SET_CNT index; stored as u32, 256 entries directly after the flag block) [CT plus mod code]**
```
9,10 Yukiko-castle floor-event progress   23-30 per-dungeon highest floor (inferred)   38 Reaper/chest counter (confirmed by both)
56 Cafe visits today   62,63 map unlock   124 Play time   127 Difficulty   169 used with weather (Okina exit)
215 Caught bugs today  242,249 dungeon helper menus
```
One mod calls `SET_CNT(999, …)`, which is outside the 256-entry range: either a mod bug or the range is wrong. Social link points, date and money are **not** counts; they live in their own records.

## 3. Enums
**Social link IDs (cmmId)** [tinyadditions `SocialLink`, mod menu, cmm.tsv, Access mod all agree]
```
0 Reserve 1 IT/Fool 2 Nanako/Justice 3 Rise 4 RiseRomance (Lovers) 5 Yukiko 6 YukikoRomance (Priestess)
7 Yosuke/Magician 8 Dojima/Hierophant 9 Sayoko/Devil 10 Kanji/Emperor 11 Chie 12 ChieRomance (Chariot)
13 Fox/Hermit 14 Naoto 15 NaotoRomance (Fortune) 16 Kou 17 Daisuke (Strength) 18 Naoki/Hanged Man
19 Hisano/Death 20 Margaret/Empress 21 Ai 22 AiRomance (Moon) 23 Shu/Tower 24 Teddie/Star
25 Yumi 26 YumiRomance 27 Ayane 28 AyaneRomance (Sun) 29 Eri/Temperance 30 Seekers of Truth/Judgement
31 Adachi Jester 32 Adachi Hunger 33 Marie 34 MarieRomance (Aeon)
```
**Arcana** [arcana.tsv]
```
1 Fool ... 21 Judgement (tarot order), 22 Aeon(?), 24 World, 25 Jester, 26 Hunger, 27 Aeon (the one used), 28 Outsider
```
**Party units:** `Protagonist=1 Yosuke=2 Chie=3 Yukiko=4 Rise=5 Kanji=6 Naoto=7 Teddie=8`. The wider character list (Dojima 16 … Marie 32) is in char.tsv.

**Personas:** IDs 0–255. `persona.tsv` gives arcana (`ar`), base level (`lv`), stats and skills, e.g. `1 Izanagi ar=1 lv=1` and `248 Laksmi ar=27`. Placeholder slots: 43, 52, 180–181, 214–223, 226–231 (debug), 250–255. 189 Izanagi-no-Okami is marked "New Game Plus".

**Skills:** `BattleSkill` 0–575 in the AI enums, and 0–623 on the Amicitia skills page.

**Items:** IDs 0–2559 (0x9FF), grouped by the high byte. Full lists are in item.tsv and P4GEnums.cs.
```
0x0xx weapons | 0x1xx armor | 0x2xx accessories | 0x3xx consumables (bugs 0x38C-0x393, quest 0x3D2-0x3EB, fish 0x3EC-0x3F8)
0x4xx key items (books 0x470-0x4FE, SL items 0x4A0-0x4B9, shelf 0x4D2-0x4DE) | 0x5xx materials | 0x6xx skill cards
0x7xx clothes/costumes | 0x8xx P4G key/shelf/vegetable/gem items | 0x9xx P4G weapons incl. Hollow Forest
```

## 4. Calling flowscript from injected code on P4G PC 64-bit
- **No public P4G 64 library that calls flow functions from code exists.** `p4g64.lib` only exposes a "MetaController". The options below are ordered by how proven they are.
- **(a) Inject flowscript with BF Emulator** (`reloaded.universal.fileemulationframework.bf`). This is how the 64-bit mod menu works.
  - Put files at `FEmulator/BF/<bfname>.flow` and `.msg`.
  - Hook an existing procedure by naming yours `<proc>_hook`. Pin a procedure index with `_index_x`. Include only new or hooked procedures, not the whole decompiled script.
  - `Functions.json` and `Enums.json` can rename or override **unused** function slots.
  - From code: `_modLoader.GetController<IBfEmulator>()`, then `AddFile(path, route)` or `AddDirectory(dir)`.
  - Caveat: the compiler is not always 100% accurate when recompiling.
- **(b) Native custom function [proven in P3P PC; P4G 64 likely similar but unverified].** This is p3ppc.moviePlayer's method:
  - Find the group table with `48 8D 2D ?? ?? ?? ?? 48 C1 F8 0C` (lea, then `sar 12`).
  - Layouts: `FlowFunctionGroup{FlowFunction* Functions; int NumFunctions; int Unk}` and `FlowFunction{void* Function; int NumArgs; int Unk}`.
  - Overwrite an unused slot, e.g. `GetFlowFunction(0,4)`, with your function pointer and `NumArgs`.
  - Read arguments with the game's `GetFlowInput(int)`. Return 0 to be called again next frame, 1 when done.
  - P4G free slots: 0x0004 and 0x0005 are null in the vanilla P4 table; 0x303E, 0x303F and 0x3046 are "Null pointer" in P4G.
- **(c) Call existing handlers [P5R pattern only].** p5rpc.lib's FlowCaller:
  - Builds a fake flow context with NumArgs, typed args and a return slot, swaps the game's current-context global, calls the handler, then reads the return value.
  - Calls that yield are queued onto the main loop.
  - P5R context offsets: `NumArgs +0x2C`, `ArgTypes +0x30`, `Arguments +0x58`, `IntReturnValue +0x1D8`, `WaitingFlag +0x224`. These are P5R-specific; P4G's layout is unknown.
- **Dispatch mechanism [PS2]:** opcode `COMM` decodes `group = operand>>12`, `entry = operand&0xFFF`. The group table holds `{cmds,count}`; each entry is `{func,paramCount}`. Handlers read arguments with `scrGetParamInt/Float/String` and return with `scrSetReturnInt`.
- **32-bit only:** tinyadditions' CustomItems finds a procedure by name in the loaded BF (proc count at +40, 32-byte names from +112) and patches the procedure ID that the game's own `dng_escape` (Goho-M) call pushes. A neat way to make the game run a custom procedure itself.
- **64-bit addresses [Access mod; ASLR is off, but these change between builds]:**
  - Party/save block pointer: `*(0x141165900)`, signature `48 8B 15 ?? ?? ?? ?? 41 0F 28 FA`. Persona stock at `+0xA34 + slot*0x30` (12 slots).
  - Wallet: `0x1451BCD70`.
  - Social link kind table: `*(0x1411A6B10)`, stride 0x64, arcana byte at `+0x10`.
  - Time block: signature `48 8B 05 ?? ?? ?? ?? 66 89 58 08`. Fields: `+0 Day (0 = Apr 1)`, `+2 DayTime`, `+8 NextDay`, `+0xA NextDayTime`.
  - Field major/minor: signature `48 8B 05 ?? ?? ?? ?? 8B 08 83 F9 17 75`.
  - Persona species table: `*(0x140EC0958)`, stride 0xE, arcana at `+2`.
  - Task finder `FUN_1404FB910`; task `cmmRankUp` (work `+4` cmmId, `+8` rank).
- **32-bit layout, relative to yen [CT]:** party slots +4/+6/+8, items +0x30 (1 byte per ID), stock +0xA34, flags +0x119C, counts +0x14DC, time +0x18DC, social links +0x18F0 (23 × 0x10 records: `u16 id, u16 rank, u16 affinity`), compendium +0x2510.
  - It's plausible that the 64-bit block keeps these offsets, i.e. that `*(0x141165900)` equals the yen address. I haven't verified that.
  - The mod menu's memory-edit constant `81646940` is from the 32-bit build. It is stale even in the 64-bit copy.

## 5. How the game's own scripts rank up a social link (reconstructed)
I found no published decompile of a P4G social link event script, so this is built from the PS2 code, the mod menus and the 64-bit task data. Verify it by decompiling a real event script (likely under `event_data`; Yosuke rank 2 is event `550_020`) with `-Library P4G`.

1. **Points from dialogue choices.** The .msg option carries `[slAffinity id amt model]`. In flowscript the equivalent is `EVT_FUNCTION_0031(id, pts, x)`. The game scales points by an arcana bonus: ×1.51 with a matching Persona in the 32-bit code, and `arcanaMul` in the PS2 code.
2. **Check.** `CHECK_IF_SL_LVLUP(id)` compares points against the threshold for the current rank in the cmm table.
3. **Visual.** `SL_LVLUP_VISUAL(id, newRank, x)`. Kind values: 0 = "Established", 1–9 = rank up, 10 = MAX, 11 = reversed, 12 = broken, 13 = arcana change. It blocks the script until done and **does not write the rank**.
4. **Commit.** `INCREASE_SL(id)` sets rank+1, zeroes points and refreshes the rank flags.

The mod menu's "Increase SL" follows exactly this order:
```
EVT_FUNCTION_0031( major, 50, 1 );
SL_LVLUP_VISUAL(major, (GET_SL_LEVEL(major) + 1), 1);
INCREASE_SL(major);
```
- **Reverse / break:** `SL_LVLUP_VISUAL(id,11,1); REVERSE_SL(id,1);` and `SL_LVLUP_VISUAL(id,12,1); BREAK_SL(id,1);`.
- **Start a link (uncertain):** the mod menu uses `EVT_FUNCTION_0016(id)` twice, then `EVT_FUNCTION_0016(0)`.
- **How the romance choice is recorded:**
  - The link record's ID is switched to the romance row. PS2 `cmmCommunityAdvanceForm` (via 0x3014) maps 3→4, 5→6, 11→12, 14→15, 21→22, 25→26, 27→28; P4G presumably adds 33→34.
  - A romance story flag is set (2383–2390, 2621), probably via a `[flag …]` on the chosen dialogue option.
  - To detect lover status, use `GET_SL_LEVEL(romanceId) > 0` or `EVT_FUNCTION_0002(romanceId)`.
  - The romance-route flag numbers come from the 32-bit CT. I haven't confirmed them on 64-bit.

Two warnings. The Tupelov Atlus-Library and Persona-Library repos are obsolete copies of the same library. The GitHub "persona-4" topic page also lists repos claiming to be leaked "Persona 4 Revival" builds; these look like malware bait, so avoid them.

Sources:
- [Atlus-Script-Tools](https://github.com/tge-was-taken/Atlus-Script-Tools) and its [Persona-Modding fork](https://github.com/Persona-Modding/Atlus-Script-Tools)
- [ShrineFox P4G Mod Menu](https://github.com/ShrineFox/Persona-4-Golden-Mod-Menu)
- [p4g64.customSubMenu](https://github.com/AnimatedSwine37/p4g64.customSubMenu)
- [Find-a-Friend](https://github.com/AnimatedSwine37/Find-a-Friend)
- [p4gpc.tinyadditions](https://github.com/AnimatedSwine37/p4gpc.tinyadditions)
- [p4g64.lib](https://github.com/AnimatedSwine37/p4g64.lib)
- [p3ppc.moviePlayer](https://github.com/AnimatedSwine37/p3ppc.moviePlayer)
- [p5rpc.lib](https://github.com/AnimatedSwine37/p5rpc.lib)
- [zarroboogs p4g-ct](https://github.com/zarroboogs/p4g-ct)
- [Persona 4 Golden Access](https://github.com/AquaRose7/Persona-4-Golden-Access)
- [Persona4-Decompilation (PS2)](https://github.com/Raikaru/Persona4-Decompilation)
- [BF Emulator docs](https://sewer56.dev/FileEmulationFramework/emulators/bf.html)
- [ShrineFox library functions](https://docs.shrinefox.com/flowscript/library-functions)
- Amicitia: [P4G](https://amicitia.miraheze.org/wiki/Persona_4_Golden), [Personas](https://amicitia.miraheze.org/wiki/Persona_4_Golden/Personas), [Items](https://amicitia.miraheze.org/wiki/Persona_4_Golden/Items), [Skills](https://amicitia.miraheze.org/wiki/Persona_4_Golden/Skills), [Quests](https://amicitia.miraheze.org/wiki/Persona_4_Golden/Quests), [Events](https://amicitia.miraheze.org/wiki/Persona_4_Golden/Events), [Save layout](https://amicitia.miraheze.org/wiki/Persona_4_Golden/data00xx.bin)
- [Social Link Cheat Sheet 64](https://gamebanana.com/mods/427555)
- [Tupelov Atlus-Library](https://github.com/Tupelov/Atlus-Library)
