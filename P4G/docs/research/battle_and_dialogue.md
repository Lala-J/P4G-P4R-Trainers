# Battle internals and dialogue auto-advance: prior art

> Research notes compiled while planning the trainer (November 2026), from public mods,
> cheat tables, wikis and read-only inspection of the Steam build (P4G.exe v1.0.5,
> PE timestamp 0x64074B8B). "Your exe" / "your install" means that build. Anything not
> marked as verified must be checked against the game before the trainer relies on it;
> verified facts end up in `src/dll/Engine/Addresses.h` with a provenance tag.

I found usable 64-bit Steam material for most features. The best sources are a June 2026 Cheat Engine table by qSleeP (64-bit hooks), AquaRose7's screen-reader mod, and AnimatedSwine37's 64-bit mods. Features 3, 6 and 7 still need your own reverse engineering. I did not download or run any archives. I did read three Cheat Engine `.CT` files from FearLess as plain XML text without saving them.

**About builds:** repos named `p4gpc.*` and the 2020 tables target the 32-bit build. `p4g64.*` repos, AquaRose7, and qSleeP's table target 64-bit Steam. AquaRose7 uses the same battle manager global you already have (0x140EC08F0), so their hard-coded addresses should match your v1.0.5 exe. The AOBs below were collected from the sources, not checked against your exe — confirm each is unique before use.

## Shared structures (64-bit)
- **Battle manager** = `*(0x140EC08F0)`.
  - `+0xC` is a status byte; bit 0x80 means the outcome is decided.
  - `+0x1D0` / `+0x1D8` are the party and enemy node lists. Each node has the unit at `+0xCF0`, flags at `+0x9C` (0x8 = active) and next at `+0xD00`.
  - Source: qSleeP CT (FearLess topic 39689, file id 81565).
- **Unit stat node** (party ones are the permanent records `0x1451BD9E4 + i*0x84`):
  - `+8` HP, `+0xA` SP.
  - `+0xC` status u32: 0x80000 dead or removed, 0x100000 Down, 0x1 Dizzy, 0x2 Rage, 0x4 Fear, 0x8 Silence, 0x20 Poison, 0x80 Enervation. Panic/Confuse and Exhaustion are unconfirmed (probably 0x10 / 0x40).
  - Buffs: stage nibbles at `+0x1C..0x1F` and timers at `+0x25..0x28`. Attack is 0x1C high nibble, defense 0x1D high, hit 0x1D low, evasion 0x1E low, crit 0x1F. Nibble 0xE means a debuff (bit 3 = down).
  - `+0x16` holds charges/shields (Tetrakarn 2, Makarakarn 4, Charge 8, Mind Charge 0x10). `+0x56` is the ally's persona id.
  - Source: AquaRose7 `database/BATTLE_SYSTEM.md` and `Battle.cs`.
- **Turn and menu:** `BtlInfo+0xCB8` → Turn; `Turn+0x38` → unit; `+0xAC` command state; `+0xAE` skill used. Command-state enum: Attack 1, Skill 2, Item 3, Tactics 4, Persona 5, Escape 6, Guard 7, Analysis 0xB (AnimatedSwine37 `p4g64.quickBattleRestart`).

## 1. Unlimited Persona swaps
- **32-bit (2021):** rirurin `p4gpc.infinitepersonaswitcher`, `Switcher.cs`. It scans `F7 46 ?? ?? ?? ?? ?? 74 AF` (test of a bit flag at `[esi+x]`) and turns the `je` at +7 into `jo` (`70 AF`, never taken), so the once-per-turn check never blocks. Optional instant-switch patches: `0F B7 7B 78 BA 0C 00 00 00 8B 73 38` and `A1 ?? ?? ?? ?? 8B 53 38 6A 00 6A 00`.
- **64-bit:** SCArkadia's "Infinite Persona Switcher" (gamebanana.com/mods/590347, April 2025) ports rirurin's signature as an Inaba Exe Patcher patch. It is only shipped inside a .7z, so I did not inspect it.
- **64-bit lead:** `Btl::UI::Command::CanChangeToState(BtlTurnInfo*, short state)`, sig `48 89 5C 24 ?? 57 48 83 EC 20 48 8B F9 0F B7 DA 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 4C 8B 05 ?? ?? ?? ??`. Forcing it to return true for state 5 is a possible approach. The game itself uses this function to decide whether Escape is allowed.
- **Other 64-bit lead:** the "Persona state dispatcher" at 0x1400ECE09 (`0F B7 F0 8B CE 83 E9 01`), from seyleonhart `p4g64.P5Controls64`, `GameAddresses.cs`.
- **Pitfall:** the swap may also check a flag elsewhere, not only the menu greying. Look for the 64-bit version of the `test [reg+x], imm32 / je` pattern.

## 2. No knockdown, ailments, instant death or debuffs
No mod does this; it needs RE. Two workable approaches from the data above:
- **Every frame:** clear status bits 0x100000 and the ailment bits from 0x3F/0xFF on the party stat nodes. Also clear any debuff nibbles (value 0xE) and their timers. Leave 0x80000 (dead) alone.
- **Use the game's own logic:** the persona affinity table is `*(0x140EC0988)`, 32 bytes per persona id, 16 u16 values in this order: Phys, Fire, Ice, Elec, Wind, Almighty, Light, Dark, Panic, Poison, Fear, Rage, Dizzy, Exhaustion, Enervation, Silence.
  - High byte is a flag: 0x01 null, 0x02 repel, 0x08 weak, 0x10 resist (0x04 and 0x20 are guesses).
  - Setting Light, Dark and the eight ailments to null on your party's personas gives immunity to Hama/Mudo and ailments. Clearing the weak flags removes weakness knockdowns.
  - The table is shared data, so restore it afterwards.
- **Pitfall:** crits still knock you down. Scripted fights put units on the heap instead of the fixed records (qSleeP saw this in the Hanamura-style boss fight), so walk the `+0x1D0` party list instead of using static addresses.

## 3 and 7. Enemies always miss / party never misses
Unknown — needs RE. Two leads:
- The crit site (feature 5) has a branch that returns `eax=2` for a crit. That suggests a result value of maybe 0 = miss, 1 = hit, 2 = crit in the same function. Unverified.
- The hit and evasion buff nibbles (0x1D low, 0x1E low) only shift the odds; they do not guarantee anything.
- The FearLess tables have requests for this but no scripts.

## 4. Stop enemies fleeing
- Treasure shadows run away through the AI script function `AI_RUN_AWAY`, index 0x203F in the AI module (Atlus-Script-Tools `Persona4Golden/Modules/AI/Functions.json`). You could swap its native handler for the `AI_WAIT` (0x2040) or `AI_ATTACK` (0x203D) handler. The address of the AI function table is not published.
- **Second path:** Fear (0x4) makes enemies flee on their own. The engine then sets 0x80000 while HP is still above 0 (AquaRose7). Block Fear on enemies, or catch the dead-with-HP transition.

## 5. Party always crits
- **64-bit, working:** qSleeP table, scripts "~100% Crit MC" and "~100% Crit PARTY".
  - AOB `45 3B C6 7C 0D` = `cmp r8d,r14d` (roll vs crit chance); `jl` to `P4G.exe+D50F8`. The site works out to about 0x1400D50E6.
  - The script checks whether the attacker (`rsi`, a stat node) lies in `[0x1451BD9E4, 0x1451BDE04)`. If so, it jumps straight to 0x1400D50F8.
- **32-bit only:** `F3 0F 2C C0 89 45 0C` (writes 100 to `[ebp+0C]`), posted in the same topic.
- **Pitfall:** the static address range fails in scripted fights with heap units.

## 6. "1 More" on every action
Unknown — needs RE. The action executor is in the protected region; AquaRose7 found that the resolver at `FUN_14004D0A0` never fires. Possible starting points: P5Controls64's "Native action-state hook" at 0x14005FC72 (`66 89 5F 0C C7 47 24 00 00 00 00`) and its "actor availability" check at 0x140099278 (`80 B8 A2 00 00 00 00`). Forcing crits (feature 5) knocks enemies down and gives a 1 More, except on targets that can't be knocked down.

## 8. Affinity resolution and analysis knowledge
- `GetAffinityResult(statNode, element)` at **0x1400D35B0** returns a packed u32: low 16 bits are the damage multiplier %, plus flags 0x08000000 weak, 0x10000000 resist, 0x01000000 null, 0x02000000 repel. `FUN_1400D2840` zeroes damage for the 0x27000000 group (null/repel/drain).
- Element ids: 0 Phys, 1 Fire, 2 Ice, 3 Elec, 4 Wind, 5 Almighty, 6 Light, 7 Dark.
- Hooking this function for enemy targets is the natural place to force a weakness. Not yet proven to be what damage calculation uses.
- `GetAffinityKnown(nameId, element)` at **0x1400993F0** reads the global analysis bitmap; `nameId` is `unit+0xA4`. The game sets these flags the moment a skill resolves. The function that writes them is not published; look for writers to that bitmap.
- Other helpers: enemy table `*(0x140EC0920)+id*0x3C`; analyze panel render at 0x1400E0E50; enemy skill array from `FUN_1400D4320`.

## 9. Auto-advance dialogue
- **Existing mod:** AnimatedSwine37 `p4gpc.tinyadditions` → `AutoAdvanceToggle.cs` (32-bit) just flips bitflag **59**, which is the game's own auto-text setting. 64-bit has a native "Auto Text" config option (AquaRose7 `ConfigMenu.cs`). Calling `BIT_ON/BIT_OFF(59)` through your flowscript tables is the simplest route, but confirm the flag index on 64-bit. I found no 64-bit auto-advance mod; GameBanana "Silent Text Advance" (575434) only mutes the advance sound.
- **Message opcodes** (Atlus-Script-Tools P4G `MessageScriptLibrary.json`): `[f 1 1]` w = wait for input, `[f 1 2]` auto after a set time, `[f 1 3]` vAuto = advance when the voice line ends, `[f 3 1]` vp = play voice cue.
- **64-bit message-window hooks** (AnimatedSwine37 `p4g64.accessibility`):

| Function | Signature |
|---|---|
| `MsgWindow::DrawDialog` | `48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 83 EC 20 48 8B D9 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B7 05 ?? ?? ?? ??` |
| `MsgWindow::StartDialog(execId, msgId)` | `48 89 5C 24 ?? 55 56 57 41 55 41 57 48 83 EC 40` |
| Dialog executions table | `48 8D 15 ?? ?? ?? ?? 48 C1 E0 06 48 8B 04 ??` (stride 0x40, `+8` → info) |

  The info struct has speaker at `+0x20`, text at `+0x38`, CurrentPage `+0x48`, PageCount `+0x4A`, selection text `+0x70`, selected option `+0x7E`, option count `+0x82`.
- **Voice (CRI ADX2):** T-PoseRatkechi's Ryo has a "p4g" pattern set (`CriAtomExPatterns.cs`). It covers `criAtomExPlayer_GetStatus` (`48 89 5C 24 ?? 57 48 83 EC 20 48 8B F9 48 85 C9 75 ?? 44 8D 41 ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8D 47`), `criAtomExPlayback_GetTimeSyncedWithAudioMicro`, `SetCueId`, and `Start`. Standard CRI status values: 2 = playing, 3 = play ended.
- **Still unknown:** the window's "waiting for input" and "text fully drawn" fields. Easiest options: set flag 59 and let the engine time voices, or inject a confirm press with seyleonhart's `p4g64.InputLibrary64`.

## ShrineFox Persona-4-Golden-Mod-Menu inventory
This is 32-bit/Aemulus-era flowscript (last pushed September 2022). It replaces the Square-button menu. Contents:
- **Call Menu:** Call Field, Event, Facility, Battle (manual id, or Boss Select with 16 bosses from Shadow Yosuke to Marie), Floor, Dungeon, Cutscene, Play Credits, BGM, SFX.
- **Facilities:** TV Studio, Town Map, Daidara, Shiroku Shop, Shiroku Pub, Velvet Room, Junes Foodcourt, Crocofur, Book Shop, Produce Shop.
- **Player Menu:** Change Name, Change Date, Change Yen, Change Items, Loop Current Day, Social Links (level up, reverse, break, change, add), Social Stats (each stat or max all), Party Members (add/remove, then reload floor), Refill HP/SP.
- **Flags:** Edit Flag, Mass Flag Edit, Edit Count, Edit Memory (raw read/write), Complete All Quests, Renew All Quests, Turn Off Tired Flags, Unlock Entire Town Map, Toggle Date Display.
- **Bundled mods:**
  - Dungeon Options: return to entrance, select floor, next/previous floor, organize party, save, strategize/tactics, call the Fox, go to Marie/shops/Velvet Room/home, a "Debug Mode" entry.
  - QuickTravelPlus and MobileCalendar: school and town travel, calendar, "Spend time with…", save.
  - FindAFriend, ConsistentReaper, IntroSkip, VRGameOverSkip, SaveEveryday, RemoveLunchtimeInvites.
- **Flowscript functions it uses:** `CALL_FIELD`, `CALL_EVENT`, `CALL_BATTLE`, `CALL_FLOOR`, `CALL_DUNGEON`, `NEXT_DATE`, `ADD_YEN`, `SET_ITEM`, `ADD_PARTY`/`RM_PARTY`, `INCREASE_SL`/`REVERSE_SL`/`BREAK_SL`, `BIT_ON`/`BIT_OFF`/`SET_CNT`, `SET_HP`/`SET_SP`.

Checked with nothing relevant to these features: zarroboogs `p4g-ct` (32-bit, 2020), zachillios' FearLess table (32-bit), DeathChaos25 (level cap only), tge's 32-bit `p4gpc.modloader`, and the Vita patches. I avoided the accounts you flagged.

Sources:
- [AnimatedSwine37/p4g64.accessibility](https://github.com/AnimatedSwine37/p4g64.accessibility)
- [AnimatedSwine37/p4g64.quickBattleRestart](https://github.com/AnimatedSwine37/p4g64.quickBattleRestart)
- [AnimatedSwine37/p4gpc.tinyadditions](https://github.com/AnimatedSwine37/p4gpc.tinyadditions)
- [AquaRose7/Persona-4-Golden-Access](https://github.com/AquaRose7/Persona-4-Golden-Access)
- [qSleeP 64-bit table thread](https://fearlessrevolution.com/viewtopic.php?f=4&t=39689)
- [zachillios P4G table thread](https://fearlessrevolution.com/viewtopic.php?f=4&t=12819)
- [rirurin/p4gpc.infinitepersonaswitcher](https://github.com/rirurin/p4gpc.infinitepersonaswitcher)
- [GameBanana 590347](https://gamebanana.com/mods/590347)
- [seyleonhart/p4g64.P5Controls64](https://github.com/seyleonhart/p4g64.P5Controls64)
- [T-PoseRatkechi/Ryo](https://github.com/T-PoseRatkechi/Ryo)
- [tge-was-taken/Atlus-Script-Tools](https://github.com/tge-was-taken/Atlus-Script-Tools)
- [ShrineFox/Persona-4-Golden-Mod-Menu](https://github.com/ShrineFox/Persona-4-Golden-Mod-Menu)
- [zarroboogs/p4g-ct](https://github.com/zarroboogs/p4g-ct)
- [GamePressure table page](https://www.gamepressure.com/download/persona-4-golden-cheat-table-ct-v8-mod/z3145f3)
