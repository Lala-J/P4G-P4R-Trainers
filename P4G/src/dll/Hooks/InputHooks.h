#pragma once
// Game input routing.
//   P4G reads the keyboard and mouse through DirectInput8, the cursor position through
//   GetCursorPos, and pads through DirectInput8 or the Steam Input API, then merges all of
//   it into one PS Vita style pad read. While the trainer is open, all of that is withheld
//   from the game (at the device level and at that merged read) and given to the overlay
//   instead. Keys, buttons and clicks used inside the overlay are latched so they don't leak
//   into the game when it closes. Key releases are always passed through, so a key held
//   when the overlay opened can never stay stuck.

#include <windows.h>
#include <Xinput.h>

#include <cstdint>

namespace Input {

bool Install();                 // creates the DInput8 / XInput / cursor hooks
bool InstallGameHooks();        // the game's merged pad read (needs the "gameinput" probes)
uint32_t GamePadHeld();         // buttons held in the game's pad read (Vita layout), unfiltered
void PollLateModules();         // hooks XInput DLLs the game loads later (render thread)
bool InstallWndProc(HWND hwnd); // subclass the game window
bool RestoreWndProc();          // false if someone subclassed after us (unload unsafe)

// Our own pad read (bypasses the game-facing filter). Returns false if no pad.
bool ReadPad(XINPUT_STATE* out);

// Replays window messages captured on the window thread into ImGui (render thread).
void DrainToImGui();

// Mouse wheel. The game's message pump doesn't deliver WM_MOUSEWHEEL reliably, so a low-level
// mouse hook on the trainer's own thread collects wheel turns over the game window while the
// overlay is open. TakeWheel returns the notches since the last call (render thread).
void StartWheel();
void StopWheel();               // before unloading
float TakeWheel(float* horizontal);

// Presses the game's Confirm key (from its own key bindings) for the next keyboard poll.
void InjectConfirm();

void OnOverlayOpened();
void OnOverlayClosed();

// Verbose input logging, enabled when %LOCALAPPDATA%\P4GTrainer\debug-input exists.
extern bool g_debug;

// Virtual key that toggles the overlay (default VK_INSERT), and the controller combo.
void SetToggleKey(int vk);
int ToggleKey();

struct KeyOption {
    int vk;
    int dik;            // DirectInput scan code, to compare with the game's own key bindings
    const char* name;
};
const KeyOption* ToggleKeyOptions(int* count);
const char* ToggleKeyName();
bool GameUsesKey(int dik);      // bound to an action in the game's own key config (P4G.ini)
bool GameUsesKeyInDialogue(int dik);  // bound to Up, Down, Confirm or Cancel
int ToggleKeyDik();             // the trainer's open/close key as a DirectInput scan code

// Auto-advance's on/off key (DirectInput scan code, 0 = none). While "live" (a dialogue page is
// showing and the trainer is closed) its presses are counted and kept from the game.
void SetAutoAdvanceKey(int dik);
void SetAutoAdvanceKeyLive(bool live);
int TakeAutoAdvanceKeyPresses();

struct PadCombo {
    WORD mask;
    const char* name;
};
const PadCombo* PadCombos(int* count);
void SetPadCombo(int index);
int PadComboIndex();
WORD PadComboMask();
const char* PadComboName();

void LoadHotkeys();             // from settings.ini [input]

}  // namespace Input
