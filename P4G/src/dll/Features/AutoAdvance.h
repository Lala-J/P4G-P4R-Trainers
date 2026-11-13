#pragma once
// Auto-advance for the VN-style dialogue box.
//
//   The game's MsgWindow::DrawDialog (0x140461880) runs each frame for every open dialogue box. Its
//   info struct's low 3 bits are the page state: the game sets 4 once the page's text is fully
//   shown and it waits for Confirm. Bits 3..5 non-zero: a choice list is opening, up or picked.
//   Voice: CRI moves a player onto its active list while it plays; a sound that starts with a
//   page is that page's voice, and the trainer waits until it has stopped.
//   Advancing presses the game's own Confirm key for one input poll, only on a frame where the box
//   is still waiting — so a press can never land on a choice, a menu or anything else.

namespace AutoAdvance {

struct Options {
    bool enabled = false;
    int voicedDelayMs = 1500;    // after the voice clip ends
    int unvoicedDelayMs = 2800;  // after the page is fully shown
    int systemDelayMs = 2800;    // boxes without a speaker name
    int perCharMs = 0;           // optional extra per character for unvoiced pages
    int keyDik = 0x1E;           // on/off key during conversations (DirectInput scan code; 0 = none)
    int padButton = 0x8000;      // on/off controller button (game's Vita layout: Square = Xbox X; 0 = none)
};

Options& Opts();
void SaveOptions();
bool InstallHooks();
void Tick();  // every frame (game thread)

// For the UI: what the current box is doing.
const char* Status();

// The on/off key: choices for the picker, and the current one's name.
struct KeyChoice {
    int dik;
    const char* name;
};
const KeyChoice* KeyChoices(int* count);
const char* KeyName(int dik);
void SetKey(int dik);

// The on/off controller button, the same way (dik holds the button bit).
const KeyChoice* PadChoices(int* count);
const char* PadName(int button);
void SetPad(int button);

// The "Autoplay On" badge: 0 hidden .. 1 fully shown (it fades in and out). Shown while
// auto-advance is on, a conversation box is on screen and the trainer is closed.
float IndicatorAlpha();

}  // namespace AutoAdvance
