# Dialogue boxes and voices

Verified live, Steam build 1.0.5. Each address is in `Engine/Addresses.h`.

## Boxes: `MsgWindow::DrawDialog` `0x140461880`

Runs every frame for each open message box (the game walks its list of boxes and calls it
with each box's info struct). The info struct's first u32 holds the box flags.

| Bits | Meaning |
|---|---|
| 0..2 | Page state: 0 idle, 2 opening, 3 text typing, 4 fully shown and waiting for Confirm |
| 3..5 | Choice list: 0 none, 0x10 opening (for a few frames after Confirm on the question page; the state is still 4), 0x18 / 0x20 up, 0x28 picked |
| 8..9 | 0x100 while a page opens, 0x200 once open |
| 16 | No speaker name (narration, system text) |
| 20..23 | Box kind. The conversation box (speech and narration) has only bit 21 (0x2). Tutorial and HELP screens, whose text appears all at once, add bit 22 (0x6) and have no speaker object; battle message boxes have 0x7 or 0x8; a system pop-up had 0xF |
| 24 | Set for a frame or so as a page starts |

Info +0x20 is the speaker text object (none on HELP screens), +0x38 the page text.

Examples: `0x01200102` → `0x00200203` → `0x00200204` is a conversation page opening, typing and
waiting; `0x00200614` → `0x00200A23` is a choice list opening under a question; `0x01600102`
→ `0x00600204` is a HELP screen.

## Voices: CRI ADX2 player lists

The game plays voices, sound effects and music on a small set of shared `CriAtomExPlayer`s.
CRI keeps each player on one of two lists, both `{tail, head, count}`:

| List | Holds |
|---|---|
| `0x140BEAA58` | idle players |
| `0x140BEAA70` | players with a sound going |

Starting a sound moves the player to the second list (code at `0x14061593A`); it moves back
when the sound ends. Inside each player: +0x08 link toward the head, +0x10 toward the tail
(walk from the head, `list+8`), +0x18 the list it is on, +0x20 status (0 stopped, 1 preparing,
2 playing, 3 ended; a streamed voice stays at 1 while it plays), +0x88 the current cue,
+0x1C8 milliseconds of it played.

A page's voice starts 0.02–0.6 s after the page (or its box) starts opening, a little after
its text is fully shown. A voice line can run on across several pages of one box. The page
turn plays a short sound effect (about 0.2 s) on a shared player at the same moment, and the
choice-confirm sound runs about 1.9 s.

Neither `criAtomExPlayer_SetCueId` nor `criAtomExPlayer_Start` is called for event voices, so
the trainer watches the lists instead of hooking a start function.
