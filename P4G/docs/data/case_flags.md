# Case rescue flags

How the Case File decides that a kidnapping case is solved.

## Rule 1 — the date

If the in-game date is past a case's last day and the game is still running, that case was
solved: missing a deadline ends the game. This needs no flags and is always right.

## Rule 2 — the rescue flag (for the days before the deadline)

| Case | Flag | Status | Evidence |
|---|---|---|---|
| Yukiko | 35 | candidate | save diff below |
| others | — | not yet identified | — |

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

**To verify:**
- Watch the flags live while beating Shadow Yukiko from the pre-boss save.
- Mine the scheduler scripts for the check the 4/29 deadline makes.
