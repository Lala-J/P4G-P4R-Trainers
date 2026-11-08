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
from the grid, gets nothing, and uses it anyway. Every crash seen had the party leader just past the
end of a staircase, inside the wall cell behind it (e.g. x 17402 / y −137 when the walkable stairs
end at y −108, 360 units earlier).

- Three crashes followed a floor change made with `GOTO_REL_FLOOR` alone (no `RESET_FLOOR`) and a
  walk straight back into the arrival stairs. The floor buttons now replay the stairs procedures in
  full, which is what the game does in any case.
- One more happened on a floor entered through the gate, after a battle, with no trainer floor
  change. It could not be reproduced: ~800 random steps at the same stairs, with and without the
  trainer, never left the walkable area.

Because the cause of the second kind is not known, the trainer guards the collision step itself
(`Features/FieldGuard`): with no room it returns 0, which the game already returns when a probe
blocks a move, so nothing moves that frame instead of crashing. It logs every occurrence with the
leader's position, and the Field page tells the player how to get out. Tested by putting the leader
into the wall cell behind the 1F stairs: the guard fired 171 times in 5 seconds, no crash.
