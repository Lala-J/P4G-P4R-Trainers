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
2 playing, 3 ended), +0x88 the current cue, +0x188 the sound bank (ACB) it comes from,
+0x1C8 milliseconds of it played. Every kind of sound shows status 1 for its first moments
and 2 after that, so the status says nothing about what is playing.

A page's voice starts 0.02–0.6 s after the page (or its box) starts opening, a little after
its text is fully shown. Pressing Confirm stops it. A line can run on over a page the game turns
by itself (a timed page, no press), so a voice still playing over such a page belongs to it
too. The page turn plays a short sound effect (about 0.2 s) on a shared player at the same
moment, and the choice-confirm sound runs about 1.9 s. Voice lines run up to 23 s.

What a two-hour recording of every player (about 400 voiced lines) showed:

| Player role | Bank | Plays |
|---|---|---|
| Voice | a new bank for each scene | every voice line, back to back (the cue changes from page to page while it stays on the active list) |
| Music (three players) | one music bank, the same for the whole session | music tracks (minutes long) and short stingers (2–3 s) |
| System sound | one system bank | page-turn clicks and menu sounds |
| Scene sound (a pool) | the scene's own banks | footsteps, doors and other scene effects |

A new music track can start just as a page opens, and before the trainer knew better that held
every page up until the track was 30 s in. The trainer now counts a sound as a page's voice
only on that page (or a page the game turns while it plays), stops counting any sound once it
has run 30 s, and treats a bank as music once more than one player has played from it and one
of its sounds has run past 30 s. Only the music bank fits that: each scene's voice bank is only
ever played by the voice player. The voice player can keep a single sound going for over 30 s
(seen once, in a scene on 4/30), so marking players instead of banks went wrong: the voice player
was taken for music after that sound, and every voice after it was skipped.

A sound's length is read from CRI's own count (+0x1C8), not the time since it started: when the
game's audio pauses (the window losing focus, for one), every player stays on the active list
with its count standing still. One voice line sat at 8.2 s for 21 s that way, with the music
standing still too, and then finished at 9 s.

Neither `criAtomExPlayer_SetCueId` nor `criAtomExPlayer_Start` is called for event voices, so
the trainer watches the lists instead of hooking a start function.
