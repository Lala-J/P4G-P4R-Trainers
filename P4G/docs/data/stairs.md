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

## Why every step matters

An earlier version called only `GOTO_REL_FLOOR` (later with the fade). Without
`common_floor_change`'s `RESET_FLOOR` the game crashed within a minute or two of arriving on a
generated floor: a field job (`P4G.exe+0x32A94B`, called via `0x1402D8955`) looked up the room for
the leader's cell, found none, and dereferenced null. With the full sequence: 5 floor changes and
about 470 steps of wall-hugging right after arrival, no crash (it had crashed in 3 of 4 runs).
