# P4G / P4R Trainers

In-game trainers for **Persona 4 Golden** (Steam, PC) and, once it's out, the Persona 4
remake. The trainer opens as an overlay on top of the game. It can change practically any
game mechanic, and it's careful about it: every change goes through the game's own code
wherever possible, and anything that could affect the story asks first and explains why.

---

## Part 1 — Using the trainer

### Persona 4 Golden (Steam)

**Supported version:** the current Steam release, `P4G.exe` v1.0.5 (64-bit). If the game is
updated, the trainer recognises that it's a different build and stays switched off instead of
guessing. The Dashboard says so.

#### Install

1. Download the latest release, or build it yourself (Part 2).
2. Put `P4GTrainer.exe` and `P4GTrainer.dll` in any folder, together. The game folder is
   not required.
3. Start Persona 4 Golden. Run `P4GTrainer.exe` once you're at the title screen or in game.
   - `P4GTrainer.exe --wait` waits for the game to start and then loads.
   - `P4GTrainer.exe --eject` unloads the trainer; `--reload` swaps in a newer build.
4. In game, press **Insert**, or **Back + R3** on a controller, to open or close the trainer.
   Both can be changed on the Settings page (keys the game itself uses are never offered).

While the trainer is open the game receives no input. Inside the trainer, **LB / RB** (or
Page Up / Page Down) switch pages, and **Esc** or **Start** closes it. Mouse, keyboard and
controller all work everywhere.

Good to know:

- Changes that touch what the game is doing right now (the Persona stock, shop and Velvet Room
  shortcuts, floor changes) only run while you're free-roaming. Otherwise the button says why
  not, e.g. "Not during a battle".
- Dialogue auto-advance only ever presses Confirm in the conversation box, never on a choice,
  menu or pop-up. Holding any key pauses it.
- Toggles (battle overrides, never-run-out items, minimap reveal, auto-advance, EXP options)
  are remembered between sessions in `%LOCALAPPDATA%\P4GTrainer\settings.ini`.

The first time the trainer loads in a session it copies your save folder to
`%LOCALAPPDATA%\P4GTrainer\backups\<date-time>\`. The trainer never writes save files itself.

#### What it does

| Page | What you get | Status |
|---|---|---|
| Dashboard | Today's date and time slot, the case deadline, links ready to rank up, your stats | ✅ |
| Today | Everything you can do in the current time slot and where to find it, ordered by priority: last chances, romance, social links and hangouts, clubs and jobs, everything else | ✅ |
| Battle | Infinite HP (no damage, or never below 1) and infinite SP, per member HP/SP, refill | ✅ |
| Battle overrides | No ailments or knock-downs on the party, enemies always miss, always crit, forced 1 More, a weakness override, unlimited Persona swaps, enemies can't flee | ✅ |
| Inventory | Every item in the game by category, set counts through the game's own routine, items that never run out in battle or the camp menu | ✅ |
| Personas | Yu's stock and every party member's Persona; edit stats and skills | ✅ |
| Personas (more) | Get any Persona Yu can hold (base form), remove one, summon from the compendium (with or without the fee), save to the compendium; fuse ultimates before their link is maxed | ✅ |
| Social Links | Rank and points against the game's own thresholds for every link, "ready to rank up", locked-out pairs, spoiler-hidden unmet links | ✅ |
| Social Links (edits) | Rank-ups that also replay the link's own story effects, "ready at next hangout", with prerequisite warnings | ✅ |
| Romance | Pick a partner by portrait and follow a step-by-step route; every step stays hidden until you reveal it | ✅ |
| Case File | The current case, its deadline, days left, and the case timeline | ✅ |
| Protagonist | Level and EXP; Courage, Knowledge, Diligence, Understanding, Expression with ranks; money; equipment | ✅ |
| Levels & EXP | Levels and EXP for Yu, party members and every Persona, using the game's own level-up routines | ✅ |
| Enemy Intel | Every enemy's affinities shown in the game's own Analysis screen, plus an in-battle panel | ✅ |
| NG+ & Unlocks | New Game+ carry-overs one at a time: social stats, money, the whole compendium, the New Game+ Persona | ✅ |
| Field & Dungeon | Shops and the Velvet Room from anywhere in town, next / previous floor, the whole floor revealed on the minimap, save anywhere, auto-advance for dialogue | ✅ |
| Advanced | Undo journal for every change | ✅ |
| Advanced (more) | Browser for the game's event flags and counters, with known meanings, "changed since opened", and journaled edits | ✅ |

#### Safety

- **Risk levels.** Every control is marked *Safe*, *Caution*, *Story-sensitive* or
  *Dangerous*. The last two open a confirmation that says exactly what will change and what
  could go wrong.
- **Warnings that make sense.** The trainer warns about things that don't fit the story. For
  example, maxing a social link you haven't met yet, or one the story only advances by itself.
  It tells you what's missing, and lets you go ahead anyway.
- **Undo.** Changes are journaled and can be undone.
- **Spoilers.** Anything past your current point in the story is shown as `???` until you
  reveal that one item.
- **Things it deliberately doesn't do:** change the date, loop days, unlock the whole town map,
  "complete all quests", or start arbitrary events. They break the game's scheduler or skip
  rewards in ways that can't be repaired. It also doesn't mark a playthrough as New Game+ (the
  story checks that in many places), and it doesn't lift the fusion level cap: the game checks
  Yu's level in a dozen places, so instead you can get a higher-level Persona directly.

### Persona 4 remake (P4R)

The trainer for the remake will be here when the game comes out.

---

## Part 2 — For developers

### Layout

```
README.md, LICENSE           this file; MIT licence
tools/check_public.py        blocks commits containing local paths, account ids, emails, keys or game assets
.githooks/pre-commit         runs check_public on staged files (opt-in, see below)
P4G/
  src/dll/                   the trainer (injected DLL)
    Engine/                  Addresses.h (audited address table), HostCheck, SigScan, SaveBackup,
                             GameThread (work queued onto the game's thread), FlowCaller (calls the
                             game's own script functions), Natives, GameState (per-frame snapshot)
    Features/                one module per feature (Battle, BattleOverrides, Inventory, PersonaStock,
                             SocialLinks, Levels, FieldQol, AutoAdvance, EnemyIntel, Journal, ...)
    Data/                    small tables: links, cases, romance routes, today's activities,
                             generated link side effects (*.inc)
    Hooks/                   MinHook wrapper, D3D11 Present, input (DirectInput8, XInput, window, wheel)
    UI/                      overlay, theme, shell, portraits and pages
    Core/, Util/             lifecycle, logging, paths, settings, CPK/PAK/TMX readers
  src/injector/              P4GTrainer.exe (attach / eject / hot-swap)
  src/tests/                 headless self-test, run by the build
  tools/                     env.sh, build.sh, fetch_deps.sh
  tools/re/                  read-only reverse-engineering helpers and test drivers (screenshots,
                             keys, clicks, title screen, auto-fight; they refuse to act on save screens)
  tools/datamine/            CPK and flowscript readers that mine social link side effects from the
                             player's own data files (output: src/dll/Data/*.inc, docs/data/)
  docs/research/             research notes the address table and rules are based on
  docs/data/                 what was verified in the game: case flags, link events, minimap, ...
P4R/                         placeholder
```

### Building

Requirements: Windows 10/11, Visual Studio 2022 or newer with the *Desktop development with
C++* workload, Git for Windows (for Git Bash), and Python 3 for the checks and tools.

```bash
cd P4G
bash tools/fetch_deps.sh      # Dear ImGui and MinHook, at pinned commits, into P4G/third_party/
bash tools/build.sh release   # or: debug
```

`build.sh` finds Visual Studio with `vswhere` and the Windows SDK through the registry, so no
paths need to be configured. It builds the DLL and the injector, runs the self-test and the
public-repo check, and stages `P4G/dist/`.

To have the public-repo check run on every commit:

```bash
git config core.hooksPath .githooks
```

### How the trainer is put together

- **Fail closed.** `HostCheck` verifies the executable, its build timestamp and section layout,
  then checks each entry in `Addresses.h` against expected bytes. An unknown build leaves the
  trainer dormant. A failed probe disables only the features that depend on it.
- **Provenance.** Every address in `Addresses.h` says where it came from: `[DRV]` derived
  here by disassembly, `[MOD:<repo>]` from a public mod, `[CT:<table>]` from a Cheat Engine
  table. All of them are re-verified against the target build before use.
- **Game code first.** Changes are made by calling the game's own functions, or by replaying
  exactly what the game's scripts do. Raw memory writes are the last resort, and only for
  plain values.
- **The protector.** P4G's real code lives in `.shared`. `.arch` holds the protector's
  virtualised code. Functions that jump into `.arch` may be called but are never hooked.
- **Threads.** The UI never touches game memory directly. Changes are queued and applied on
  the game's own thread.

### Contributing

- **Keep the repo public-safe.** Don't commit absolute paths, user names, account ids, emails
  or keys. Discover locations at run time. `tools/check_public.py` enforces this.
- **No game content.** No executables, extracted files, scripts, textures or portraits. Facts
  derived from the game, like flag numbers and addresses, are fine. Names and images are read
  from the player's own installation at run time.
- **No deletions in scripts.** The build overwrites in place and moves stale files to
  `.trash/`, which is git-ignored.
- **Verify before trusting.** New addresses need a probe in `Addresses.h` and a provenance tag.
- **Test.** Build with `tools/build.sh`, then try your change in the game. Read the log in
  `%LOCALAPPDATA%\P4GTrainer\logs\`.

### Credits

Built on [Dear ImGui](https://github.com/ocornut/imgui) (MIT) and
[MinHook](https://github.com/TsudaKageyu/minhook) (BSD-2). The research behind the address
table and the game rules draws on the Persona modding community. Thanks to AnimatedSwine37,
AquaRose7, zarroboogs, tge, ShrineFox, rirurin, seyleonhart, DeathChaos25, qSleeP and the
authors of the Megami Tensei wikis. Their sources are linked in `P4G/docs/research/`.

Persona 4 Golden is © ATLUS / SEGA. This project is not affiliated with them.
