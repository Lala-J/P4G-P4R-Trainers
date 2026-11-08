#pragma once
// Auto-advance for the VN-style dialogue box.
//
//   The game's MsgWindow::DrawDialog (0x140461880) runs each frame for every open dialogue box. Its
//   info struct's low 3 bits are the page state: the game sets 4 once the page's text is fully
//   shown and it waits for Confirm. Bits 3..5 >= 0x18 mean a choice list is showing.
//   Voice: the message system starts its voice cue on a CRI player (criAtomExPlayer_SetCueId)
//   as the page opens; the trainer polls that player (criAtomExPlayer_GetStatus) until it ends.
//   Advancing presses the game's own Confirm key for one input poll, only on a frame where the box
//   is still waiting — so a press can never land on a choice, a menu or anything else.

namespace AutoAdvance {

struct Options {
    bool enabled = false;
    int voicedDelayMs = 1500;    // after the voice clip ends
    int unvoicedDelayMs = 2000;  // after the page is fully shown
    int systemDelayMs = 2000;    // boxes without a speaker name
    int perCharMs = 0;           // optional extra per character for unvoiced pages
};

Options& Opts();
void SaveOptions();
bool InstallHooks();
void Tick();  // every frame (game thread)

// For the UI: what the current box is doing.
const char* Status();

}  // namespace AutoAdvance
