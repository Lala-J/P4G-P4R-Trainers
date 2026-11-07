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

While the trainer is open the game receives no input. Inside the trainer, **LB / RB** (or
Page Up / Page Down) switch pages, and **Esc** or **Start** closes it.

The first time the trainer loads in a session it copies your save folder to
`%LOCALAPPDATA%\P4GTrainer\backups\<date-time>\`. The trainer never writes save files itself.

#### What it does

| Page | What you get | Status |
|---|---|---|
| Dashboard | Build check, controls, what's loaded | ✅ |
| Today | Everything you can do in the current time slot and where to find it, ordered by priority: last chances, romance, social links and hangouts, clubs and jobs, everything else | planned |
| Battle | Infinite HP/SP, no ailments or knock-downs on the party, enemies always miss, always crit, forced 1 More, a weakness override, unlimited Persona swaps, enemies can't flee | planned |
| Inventory | Every item in the game: add, remove, set counts; items that never run out | planned |
| Personas | Edit stock and compendium, give or remove Personas, fusion preview, Velvet Room rule overrides | planned |
| Social Links | Rank, points, availability and prerequisites for every link; rank-ups that also replay the link's own story effects | planned |
| Romance | Pick a partner by portrait and follow a step-by-step route; every step stays hidden until you reveal it | planned |
| Case File | The current case, its deadline, and the usable days left | planned |
| Protagonist | Courage, Knowledge, Diligence, Understanding, Expression, money, equipment | planned |
| Levels & EXP | Levels and EXP for Yu, party members and every Persona, using the game's own level-up routines | planned |
| Enemy Intel | Every enemy's affinities shown in the game's own Analysis screen, plus an in-battle panel | planned |
| NG+ & Unlocks | New Game+ carry-overs granted one at a time, with warnings | planned |
| Field & Dungeon | Shops from anywhere, floor select, party changes in dungeons, save anywhere, auto-advance for dialogue | planned |
| Advanced | Flag and counter browser, undo journal | planned |

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
- **Things it deliberately doesn't do:** change the date, loop days, unlock the whole map,
  "complete all quests", or start arbitrary events. They break the game's scheduler or skip
  rewards in ways that can't be repaired.

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
    Engine/                  Addresses.h (audited address table), HostCheck, SigScan, SaveBackup
    Hooks/                   MinHook wrapper, D3D11 Present, input (DirectInput8, XInput, window)
    UI/                      overlay, theme, shell and pages
    Core/, Util/             lifecycle, logging, paths
  src/injector/              P4GTrainer.exe (attach / eject / hot-swap)
  src/tests/                 headless self-test, run by the build
  tools/                     env.sh, build.sh, fetch_deps.sh
  tools/re/                  read-only reverse-engineering helpers and test drivers
  docs/research/             research notes the address table and rules are based on
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
