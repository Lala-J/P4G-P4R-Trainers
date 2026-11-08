# Case rescue flags

How the Case File decides that a kidnapping case is solved.

## Rule 1 — the date

If the in-game date is past a case's last day and the game is still running, that case was
solved: missing a deadline ends the game. This needs no flags and is always right.

## Rule 2 — the rescue flag (for the days before the deadline)

| Case | Rescue flag | Boss flag | Rescue event | Window (scheduler) | Evidence |
|---|---|---|---|---|---|
| Yukiko | 35 | 17 | E181 | 4/18 – 4/29 | scripts + **watched live** |
| Kanji | 36 | 18 | E216 | 5/18 – 6/4 | scripts |
| Rise | 37 | 19 | E254 | 6/24 – 7/9 | scripts |
| Mitsuo | 38 | 20 | E282 | 7/27 – 8/12 | scripts |
| Naoto | 39 | 21 | E309 | 9/16 – 10/5 | scripts |
| Nanako | 40 | 22 | E358 | 11/6 – 11/20 | scripts |

**The rule, from the game's own day scheduler** (`scheduler/scheduler_MM.bf`, every evening
procedure `sdlMM_DD_PM_D` in the case's window): *if the rescue flag is off and the boss flag is on,
turn the rescue flag on and play the rescue cutscene.* The boss flag is set by the boss-room event
in `field/script/dungeon.bf` (`eve_*_bossroom`). Every window ends on exactly the case's deadline.
The later dungeons (Magatsu Inaba, Hollow Forest, Yomotsu Hirasaka) have no rescue flag of this
kind; the Case File uses the date for them.

### Yukiko: before/after save diff

Two saves from the same playthrough: 4/20 at the TV entrance just before the Shadow Yukiko fight,
and 4/21 after school after clearing the castle. The flag block sits at file offset
`0x68 + 0x119C` in a save file (the file is the save block with a small header; yen is at
`0x58`, so offsets differ per region).

Flags that turned **on**: 17, 35, 1487, 1488, 2592, 2772, 3701, 5568–5585 (several).
Flags that turned **off**: 190, 1456, 1478, 1556, 1736, 1737, 2177, 2593, 2771, 3074, 3953.

Among those flags:
- **17** and **35** are the only low story flags.
- Live flags **30** and **32** are already set and look like the matching flags for Yosuke's
  and Chie's awakenings.
- That makes **35** the likely Yukiko flag. **17** is the alternative.
- The others change with the day or with dungeon progress.

### Yukiko: live confirmation

The trainer's trace monitor logged every flag change while replaying the pre-boss save through the
fight. The order of events:

| When | Flag change | Meaning |
|---|---|---|
| Opening the boss door | 17 on, 190 off | The pre-fight cutscene. **17 means "boss reached", not "rescued".** |
| During the fight | 5568–5585 on | The boss's battle script, step by step (summons, phase changes). |
| End of the rescue cutscene | **35 on**, 3701 on | **Yukiko rescued.** |

Flag 35 never turns on without the rescue, and it is set once that sequence ends, so it's the
flag the Case File uses.
