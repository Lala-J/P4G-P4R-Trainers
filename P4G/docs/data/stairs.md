# Dungeon stairs (Next / Previous floor)

From the game's own `field/script/dungeon.bf` (data_e.cpk), procedures `goto_stair` and
`backto_stair`. The trainer's floor buttons replay these steps exactly.

## goto_stair (up)

1. `floor = GET_FLOOR_ID()`
2. `common_floor_change` (procedure 87):
   - `RESET_FLOOR()` (field function 0x1003)
   - if counter 38 == 13: `SET_CNT(38, 14)`, `BIT_OFF(3118)`, `BIT_OFF(3119)`
3. `get_reach_level` (procedure 86): counters 9/10 (35/36 for floors 160..178) hold the highest
   floor reached; each one that is `<= floor` becomes `floor + 1`.
4. Stairs sound, by floor range (bit 3167 is the "stairs" flag):
   - 20..39 → system sound 27
   - 40..59, or 100..119 with bit 3167 off → 25
   - 80..99 → 26
   - 120..139 with bit 3167 off → `PLAY_SOUNDEFFECT(705)` instead
   - otherwise → 24
5. `BIT_OFF(3167)`, `PLAY_SYS_SOUNDEFFECT(1, sound)` (when a system sound was picked)
6. `FADE(1, 10)`, `FADE_SYNC()`
7. The change itself:
   - floor in {6, 12, 30, 50, 69, 88, 109, 121, 148, 169} → `CALL_DUNGEON(floor + 1, 0)`
   - floor 122 → `CALL_DUNGEON(129, 0)`
   - otherwise → `GOTO_REL_FLOOR(1)`

## backto_stair (down)

Same as above without `get_reach_level`; the change is `CALL_DUNGEON(floor - 1, 1)` for floors
{6, 8, 21, 41, 61, 81, 101, 121, 141, 161}, a special case for floor 123 (not offered by the
trainer), otherwise `GOTO_REL_FLOOR(-1)`.

## The crash this was investigated for

The game crashes (`P4G.exe+0x32A94B`, null dereference) when a field object's collision step
(`0x14032A200`) runs while the object stands in a cell that belongs to no room: it looks the room up
from the grid, gets nothing, and uses it anyway.

**Cause (confirmed live):** the back end of some staircases isn't solid. Walking into it carries the
party past the end of the stairs into the wall cell behind and on into the void (seen on Yukiko's
Castle 1F and 4F, on floors entered through the gate and through the trainer's floor buttons alike).
In the unmodified game the next collision step then crashes. It is position-dependent and rare: one
random walk of ~170 steps reached it, others of ~800 did not.

**Guard (`Features/FieldGuard`):**
- the collision step returns 0 instead of dereferencing a null room, so nothing crashes;
- every frame the trainer remembers where the leader last stood on a room cell and, the moment the
  leader stands in a wall cell, puts it back there (or in the middle of the nearest room cell if
  there is no such point yet). In practice the back end of the staircase then acts like a wall.

Verified: the party walked out on 4F (the guard caught 752 collision steps that would each have
crashed), was moved back onto the floor, and in a further 257-step walk that pushed into the same
spot 22 times was held at the edge each time. No crash.

The floor buttons replay the stairs procedures in full in any case (see above); that matches what
the game does but was not, on its own, the fix.
