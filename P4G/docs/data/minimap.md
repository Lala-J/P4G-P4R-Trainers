# Dungeon minimap

Verified live on a generated floor (Yukiko's Castle 3F), Steam build 1.0.5.

## Layout grid — `0x1411AB39C`

`cell[24][16]`, 16 bytes per cell, index `major * 16 + minor`.

| Offset | Meaning |
|---|---|
| +0x00 | 0 outside the floor, 1 walkable, 2 wall. Some floors have a wall ring around the grid, others don't (Yukiko's Castle 5F). |
| +0x01 | high nibble: position inside a multi-cell room (0 corner, 1 edge, 2 inner); low nibble: room anchor |
| +0x02 | u16 tile / room model id |
| +0x04 | room shape type (1 corridor, 2 2x2, 5 dead end, 6 junction, 8, 0x0B / 0x0C 3x3 …) |
| +0x05..0x07 | rotation, room width, room height |
| +0x0A | open sides: low nibble S 1 / E 2 / N 4 / W 8, high nibble the same for doors. 0 on the unused corners of L-shaped room footprints. |
| +0x0B | door state bits checked by the reveal pass |

The whole floor is in this grid as soon as it is built; it is not the "explored" state.
`0x1402DD44F` clears it (memset 0x1800) and draws the outer wall ring; `0x1403105FD` copies a
stored layout into it.

World position → cell: `minor = (x + 600) / 1200`, `major = (z + 600) / 1200`, where the player is
`*(0x140EC0FF0) → +0x250 → +0x48 → +0x18`, `+0x360` x, `+0x368` z.

## Explored bits — `*(0x1451EC580)`

`u16[24]`: bit `minor` of word `major` set = the cell is drawn on the minimap. Allocated once
(`0x14042F470`, 0x30 bytes) and cleared when a floor is set up (`0x1402DF0E3`, `rep stosw`).
`0x1403227E2` also checks these bits to decide whether an object on the floor is shown.

## Reveal

`0x14042FA80 (u8 minor, u8 major)` reveals one cell: it sets the cell's bit and, depending on the
room type, the bits that belong to the same room. The per-frame pass at `0x140430A03` calls it for
the player's cell and then for each neighbour whose shared side is open (and whose door, if any,
is open).

The trainer's "Reveal the whole floor" calls the same function for every walkable cell with at
least one open side, on the game thread, a few times a second. It runs only when the player's own
cell is a walkable room cell (non-zero room id) already revealed by the game, so the grid and the
bits are known to belong to the floor the player is on. Nothing is saved: floors are rebuilt on every visit.
